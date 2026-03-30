#define _USE_MATH_DEFINES
#include <Windows.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstdint>

using namespace std;

typedef struct {
    double re;
    double im;
} Complex;

void fft(vector<Complex>& x, bool invert) {
    int n = (int)x.size();

    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;

        if (i < j)
            swap(x[i], x[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * M_PI / len * (invert ? -1 : 1);
        Complex wlen = { cos(ang), sin(ang) };
        for (int i = 0; i < n; i += len) {
            Complex w = { 1, 0 };
            for (int j = 0; j < len / 2; j++) {
                Complex u = x[i + j];
                Complex v = {
                    x[i + j + len/2].re * w.re - x[i + j + len/2].im * w.im,
                    x[i + j + len/2].re * w.im + x[i + j + len/2].im * w.re
                };
                x[i + j].re = u.re + v.re;
                x[i + j].im = u.im + v.im;
                x[i + j + len/2].re = u.re - v.re;
                x[i + j + len/2].im = u.im - v.im;

                Complex nw = {
                    w.re * wlen.re - w.im * wlen.im,
                    w.re * wlen.im + w.im * wlen.re
                };
                w = nw;
            }
        }
    }

    if (invert) {
        for (int i = 0; i < n; i++) {
            x[i].re /= n;
            x[i].im /= n;
        }
    }
}

double find_freq(const vector<int16_t>& buf, int sample_rate, int fft_size) {
    vector<Complex> x(fft_size, {0, 0});
    int n = min((int)buf.size(), fft_size);
    for (int i = 0; i < n; i++) {
        x[i].re = buf[i];
    }

    fft(x, false);

    vector<double> mag(fft_size / 2);
    for (int i = 0; i < fft_size / 2; i++) {
        mag[i] = x[i].re * x[i].re + x[i].im * x[i].im;
    }

    int peak = 1;
    for (int i = 2; i < fft_size / 2; i++) {
        if (mag[i] > mag[peak]) peak = i;
    }

    return (double)peak * sample_rate / fft_size;
}

const int SAMPLE_RATE = 44100;
const int FFT_SIZE = 4096;
vector<int16_t> g_buffer;

void CALLBACK waveInProc(
    HWAVEIN hwi,
    UINT uMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
) {
    if (uMsg == WIM_DATA) {
        WAVEHDR* wh = (WAVEHDR*)dwParam1;
        int16_t* data = (int16_t*)wh->lpData;
        int samples = wh->dwBytesRecorded / 2;

        g_buffer.assign(data, data + samples);
        waveInAddBuffer(hwi, wh, sizeof(WAVEHDR));
    }
}

int main() {
    cout << "=== 实时声音频率检测 ===" << endl;
    cout << "按回车退出" << endl << endl;

    HWAVEIN hwi;
    WAVEFORMATEX fmt{};
    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = 1;
    fmt.nSamplesPerSec = SAMPLE_RATE;
    fmt.wBitsPerSample = 16;
    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    MMRESULT res = waveInOpen(&hwi, WAVE_MAPPER, &fmt, (DWORD_PTR)waveInProc, 0, CALLBACK_FUNCTION);
    if (res != MMSYSERR_NOERROR) {
        cout << "麦克风打开失败" << endl;
        return 1;
    }

    const int BUF_SAMPLES = 4096;
    vector<int16_t> wave_buf(BUF_SAMPLES);
    WAVEHDR wh{};
    wh.lpData = (LPSTR)wave_buf.data();
    wh.dwBufferLength = BUF_SAMPLES * 2;
    waveInPrepareHeader(hwi, &wh, sizeof(WAVEHDR));
    waveInAddBuffer(hwi, &wh, sizeof(WAVEHDR));
    waveInStart(hwi);

    while (!GetAsyncKeyState(VK_RETURN)) {
        if (!g_buffer.empty()) {
            double freq = find_freq(g_buffer, SAMPLE_RATE, FFT_SIZE);
            if (freq > 60 && freq < 3000) {
                cout << "当前频率: " << freq << " Hz   \r";
            }
        }
        Sleep(50);
    }

    waveInStop(hwi);
    waveInUnprepareHeader(hwi, &wh, sizeof(WAVEHDR));
    waveInClose(hwi);

    cout << endl << "已退出" << endl;
    return 0;
}