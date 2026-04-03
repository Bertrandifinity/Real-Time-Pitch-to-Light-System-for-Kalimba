#include <iostream>
#include <vector>
#include <cmath>
#include <alsa/asoundlib.h>
#include <string>
#include <cstdio>

using namespace std;

struct Note {
    string name;
    double freq;
};

const vector<Note> NOTE_TABLE = {
    {"C4", 258.398},
    {"D4", 301.465},
    {"E4", 322.998},
    {"F4", 344.531},
    {"G4", 387.598},
    {"A4", 430.664},
    {"B4", 495.264},
    {"C5", 516.797},
    {"D5", 581.396},
    {"E5", 667.529},
    {"F5", 710.596},
    {"G5", 796.729},
    {"A5", 882.861},
    {"B5", 990.527},
    {"C6", 1055.127},
    {"D6", 1184.326},
    {"E6", 1313.525}
};

const double TOLERANCE = 10.0;

struct Complex { double re = 0, im = 0; };

void fft(vector<Complex>& x, bool invert) {
    int n = x.size();
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) swap(x[i], x[j]);
    }
    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * M_PI / len * (invert ? -1 : 1);
        Complex wlen = {cos(ang), sin(ang)};
        for (int i = 0; i < n; i += len) {
            Complex w = {1, 0};
            for (int j = 0; j < len/2; j++) {
                Complex u = x[i+j];
                Complex v = {
                    x[i+j+len/2].re * w.re - x[i+j+len/2].im * w.im,
                    x[i+j+len/2].re * w.im + x[i+j+len/2].im * w.re
                };
                x[i+j].re = u.re + v.re;
                x[i+j].im = u.im + v.im;
                x[i+j+len/2].re = u.re - v.re;
                x[i+j+len/2].im = u.im - v.im;
                w = {w.re*wlen.re - w.im*wlen.im, w.re*wlen.im + w.im*wlen.re};
            }
        }
    }
    if (invert)
        for (auto& c : x) { c.re /= n; c.im /= n; }
}

double get_freq(const int16_t* buf, int samples, int sample_rate) {
    const int fft_size = 1024;
    vector<Complex> x(fft_size);
    int n = min(samples, fft_size);
    for (int i = 0; i < n; i++)
        x[i].re = buf[i];

    fft(x, false);

    double max_mag = 0;
    int peak = 1;
    for (int i = 1; i < fft_size/2; i++) {
        double mag = x[i].re*x[i].re + x[i].im*x[i].im;
        if (mag > max_mag) {
            max_mag = mag;
            peak = i;
        }
    }
    return (double)peak * sample_rate / fft_size;
}

string match_closest_note(double freq, double &standard_freq) {
    double min_diff = 1e9;
    string best_name = "";
    standard_freq = 0;

    for (const auto& note : NOTE_TABLE) {
        double diff = fabs(freq - note.freq);
        if (diff < min_diff) {
            min_diff = diff;
            best_name = note.name;
            standard_freq = note.freq;
        }
    }
    return best_name;
}

double get_rms(const int16_t* buf, int len) {
    double sum = 0;
    for (int i = 0; i < len; i++)
        sum += buf[i] * buf[i];
    return sqrt(sum / len);
}

const int SAMPLE_RATE = 44100;
const int BUF_SIZE = 1024;
const double NOISE_THRESH = 80.0;

int main() {
    cout << "=== Note Lock · Accurate Frequency Match ===" << endl;
    cout << "Press Ctrl+C to exit" << endl << endl;

    snd_pcm_t* pcm;
    int err = snd_pcm_open(&pcm, "plughw:2,0", SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) { cerr << "Failed to open microphone" << endl; return 1; }

    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm, params);
    snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, params, 1);

    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(pcm, params, &rate, nullptr);

    snd_pcm_uframes_t period = BUF_SIZE;
    snd_pcm_hw_params_set_period_size_near(pcm, params, &period, nullptr);

    snd_pcm_hw_params(pcm, params);
    snd_pcm_hw_params_free(params);

    vector<int16_t> buf(BUF_SIZE);
    snd_pcm_prepare(pcm);

    string current_note;
    double display_freq = 0;

    while (true) {
        err = snd_pcm_readi(pcm, buf.data(), BUF_SIZE);
        if (err == -EPIPE) { snd_pcm_prepare(pcm); continue; }
        if (err < 0) break;

        double rms = get_rms(buf.data(), BUF_SIZE);
        if (rms < NOISE_THRESH)
            continue;

        double real_freq = get_freq(buf.data(), BUF_SIZE, SAMPLE_RATE);
        string new_note = match_closest_note(real_freq, display_freq);

        if (!new_note.empty() && new_note != current_note) {
            current_note = new_note;
        }

        if (!current_note.empty()) {
            printf("\rCurrent Note: %-6s  Standard Freq: %.3f Hz", current_note.c_str(), display_freq);
            fflush(stdout);
        }
    }

    snd_pcm_close(pcm);
    cout << endl;
    return 0;
}
