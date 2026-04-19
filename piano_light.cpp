#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <string>
#include <alsa/asoundlib.h>
#include <fftw3.h>
#include <unistd.h>

#define DEVICE_NAME "plughw:2,0"
#define SAMPLE_RATE 48000
#define CHUNK_SIZE 4096
#define CHANNELS 1
#define VOL_THRESHOLD 0.0012

using namespace std;

atomic<bool> running(true);
atomic<int>  cmd_mode(0);
atomic<int>  cmd_speed(60);
atomic<bool> cmd_run(false);
atomic<bool> stop_led(false);

char current_note[4] = "--";
uint64_t last_note_time = 0;
const uint64_t NOTE_TIMEOUT_MS = 300;
const int LED_TIMEOUT_MS = 300;

uint64_t millis() {
    using namespace chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

void timeout_watcher() {
    while (running) {
        this_thread::sleep_for(chrono::milliseconds(5));
        if (cmd_run) {
            this_thread::sleep_for(chrono::milliseconds(LED_TIMEOUT_MS));
            stop_led = true;
        }
    }
}

void signal_handler(int) {
    running = false;
}

// ==================== Bluetooth ==========================================
void ble_thread() {
    system("hciconfig hci0 down >/dev/null 2>&1");
    system("hciconfig hci0 up >/dev/null 2>&1");
    system("hciconfig hci0 leadv 3 >/dev/null 2>&1");
    usleep(200000);

    while (running) {
        unsigned char c1 = (unsigned char)current_note[0];
        unsigned char c2 = (unsigned char)current_note[1];

        char cmd[200];
        snprintf(cmd, sizeof(cmd),
            "hcitool -i hci0 cmd 0x08 0x0008 0A 02 01 06 06 FF 00 01 %02X %02X 00 00 >/dev/null 2>&1",
            c1, c2);

        system(cmd);
        usleep(8000);
    }
}
// ==========================================================================

// ==================== Lights ==============================================
void led_thread() {
    const int leds[] = {17, 27, 22, 23, 24};
    const int n = sizeof(leds)/sizeof(int);
    thread watcher(timeout_watcher);

    while (running) {
        if (!cmd_run) {
            for (int p : leds)
                system(("pinctrl set " + to_string(p) + " dl >/dev/null 2>&1").c_str());
            this_thread::sleep_for(chrono::milliseconds(10));
            continue;
        }

        int mode = cmd_mode;
        int spd = cmd_speed;
        stop_led = false;

        if (mode == 1) {
            for (int i = 0; i < n && !stop_led; ++i) {
                for (int j = 0; j < n; ++j)
                    system(("pinctrl set " + to_string(leds[j]) + (j==i ? " dh":"dl") + " >/dev/null 2>&1").c_str());
                this_thread::sleep_for(chrono::milliseconds(spd));
            }
        }
        else if (mode == 2) {
            for (int i = 0; i < n && !stop_led; ++i) {
                int pos = n-1 - i;
                for (int j = 0; j < n; ++j)
                    system(("pinctrl set " + to_string(leds[j]) + (j==pos ? " dh":"dl") + " >/dev/null 2>&1").c_str());
                this_thread::sleep_for(chrono::milliseconds(spd));
            }
        }
        else if (mode == 3) {
            for (int i = 0; i < 3 && !stop_led; ++i) {
                for (int p : leds) system(("pinctrl set " + to_string(p) + " dh >/dev/null 2>&1").c_str());
                this_thread::sleep_for(chrono::milliseconds(spd));
                if (stop_led) break;
                for (int p : leds) system(("pinctrl set " + to_string(p) + " dl >/dev/null 2>&1").c_str());
                if (stop_led) break;
                this_thread::sleep_for(chrono::milliseconds(spd));
            }
        }

        cmd_run = false;
        stop_led = false;
        for (int p : leds)
            system(("pinctrl set " + to_string(p) + " dl >/dev/null 2>&1").c_str());
    }

    watcher.join();
}
// ======================================================================

struct NoteDef {
    string name;
    double freq;
    int mode;
    int speed;
};

const vector<NoteDef> NOTE_DICT = {
    {"C4", 261.63, 1, 100}, {"D4", 293.66, 1, 85}, {"E4", 329.63, 1, 70},
    {"F4", 349.23, 1, 55}, {"G4", 392.00, 1, 45}, {"A4", 440.00, 1, 35}, {"B4", 493.88, 1, 25},

    {"C5", 523.25, 2, 100}, {"D5", 587.33, 2, 85}, {"E5", 659.25, 2, 70},
    {"F5", 698.46, 2, 55}, {"G5", 783.99, 2, 45}, {"A5", 880.00, 2, 35}, {"B5", 987.77, 2, 25},

    {"C6",1046.50, 3, 80}, {"D6",1174.66, 3, 60}, {"E6",1318.51, 3, 40}
};

NoteDef identify(double freq) {
    double best = 1e9;
    NoteDef res{"", 0, 0, 0};
    for (auto& x : NOTE_DICT) {
        double d = fabs(freq - x.freq);
        if (d < best) {
            best = d;
            res = x;
        }
    }
    return best < 12.0 ? res : NoteDef{"", 0, 0, 0};
}

bool open_audio(snd_pcm_t*& h) {
    int err = snd_pcm_open(&h, DEVICE_NAME, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) return false;

    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(h, params);
    snd_pcm_hw_params_set_access(h, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(h, params, SND_PCM_FORMAT_S32_LE);
    snd_pcm_hw_params_set_channels(h, params, CHANNELS);

    unsigned int rate = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(h, params, &rate, 0);
    snd_pcm_hw_params(h, params);
    return snd_pcm_prepare(h) >= 0;
}

int main() {
    system("pinctrl set 17,27,22,23,24,26 op >/dev/null 2>&1");
    system("pinctrl set 26 dl >/dev/null 2>&1");
    signal(SIGINT, signal_handler);

    thread t_led(led_thread);
    thread t_ble(ble_thread);

    snd_pcm_t *pcm = nullptr;
    if (!open_audio(pcm)) {
        cerr << "mic error" << endl;
        running = false;
        t_led.join();
        t_ble.join();
        return 1;
    }

    double *in = fftw_alloc_real(CHUNK_SIZE);
    fftw_complex *out = fftw_alloc_complex(CHUNK_SIZE/2+1);
    fftw_plan plan = fftw_plan_dft_r2c_1d(CHUNK_SIZE, in, out, FFTW_MEASURE);
    vector<int32_t> buf(CHUNK_SIZE*CHANNELS);

    last_note_time = millis();
    string last_print;

    while (running) {
        int err = snd_pcm_readi(pcm, buf.data(), CHUNK_SIZE);
        if (err == -EPIPE) {
            snd_pcm_prepare(pcm);
            continue;
        }
        if (err < 0) {
            snd_pcm_close(pcm);
            this_thread::sleep_for(chrono::milliseconds(20));
            open_audio(pcm);
            continue;
        }

        double sum = 0;
        for (int i = 0; i < CHUNK_SIZE; ++i) {
            double v = buf[i] / 2147483648.0;
            sum += v*v;
            in[i] = v;
        }

        double vol = sqrt(sum / CHUNK_SIZE);
        bool detected = false;

        if (vol > VOL_THRESHOLD) {
            fftw_execute(plan);
            double max_m = 0;
            int peak = 0;
            for (int i = 0; i <= CHUNK_SIZE/2; ++i) {
                double f = (double)i * SAMPLE_RATE / CHUNK_SIZE;
                if (f < 180) continue;
                double m = out[i][0]*out[i][0] + out[i][1]*out[i][1];
                if (m > max_m) {
                    max_m = m;
                    peak = i;
                }
            }
            double freq = (double)peak * SAMPLE_RATE / CHUNK_SIZE;
            auto note = identify(freq);

            if (!note.name.empty()) {
                // 实时更新音符给蓝牙
                current_note[0] = note.name[0];
                current_note[1] = note.name[1];
                current_note[2] = 0;

                last_note_time = millis();
                detected = true;

                if (!cmd_run) {
                    cmd_mode = note.mode;
                    cmd_speed = note.speed;
                    cmd_run = true;
                }

                if (note.name != last_print) {
                    printf("\rNote: %-3s    ", note.name.c_str());
                    last_print = note.name;
                }
            }
        }

        if (!detected) {
            if (millis() - last_note_time > NOTE_TIMEOUT_MS) {
                if (last_print != "--") {
                    printf("\rSilent       ");
                    last_print = "--";
                }
                strcpy(current_note, "--");
            }
        }

        fflush(stdout);
    }

    running = false;
    t_led.join();
    t_ble.join();

    snd_pcm_close(pcm);
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    return 0;
}
