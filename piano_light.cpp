#include <iostream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <string>
#include <alsa/asoundlib.h>
#include <fftw3.h>

#define DEVICE_NAME "plughw:2,0"
#define SAMPLE_RATE 48000
#define CHUNK_SIZE 4096
#define CHANNELS 2
#define VOL_THRESHOLD 0.0012

std::atomic<bool> running(true);

std::atomic<int> anim_mode(0);
std::atomic<int> anim_speed(100);

void signal_handler(int signum) { running = false; }

void led_animation_thread() {
    int leds[5] = {17, 27, 22, 23, 24};
    int step = 0;

    system("pinctrl set 17,27,22,23,24 dl > /dev/null 2>&1");

    while (running) {
        int mode = anim_mode.load();
        int speed = anim_speed.load();

        if (mode == 0) {
            system("pinctrl set 17,27,22,23,24 dl > /dev/null 2>&1");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            step = 0;
            continue;
        }

        std::string dh_pins = "";
        std::string dl_pins = "";

        if (mode == 1) {
            // 左 → 右跑马
            for(int i = 0; i < 5; i++) {
                if (i == step) dh_pins += std::to_string(leds[i]) + ",";
                else dl_pins += std::to_string(leds[i]) + ",";
            }
            step = (step + 1) % 5;

        } else if (mode == 2) {
            // 右 → 左跑马
            for(int i = 0; i < 5; i++) {
                if (4 - i == step) dh_pins += std::to_string(leds[i]) + ",";
                else dl_pins += std::to_string(leds[i]) + ",";
            }
            step = (step + 1) % 5;

        } else if (mode == 3) {
            // 全体闪烁
            if (step % 2 == 0) dh_pins = "17,27,22,23,24,";
            else dl_pins = "17,27,22,23,24,";
            step = (step + 1) % 2;
        }

        if (!dh_pins.empty()) dh_pins.pop_back();
        if (!dl_pins.empty()) dl_pins.pop_back();

        std::string cmd = "";
        if (!dh_pins.empty()) {
            cmd += "pinctrl set " + dh_pins + " dh > /dev/null 2>&1 ; ";
        }
        if (!dl_pins.empty()) {
            cmd += "pinctrl set " + dl_pins + " dl > /dev/null 2>&1";
        }

        system(cmd.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(speed));
    }

    system("pinctrl set 17,27,22,23,24 dl > /dev/null 2>&1");
}

struct NoteDef {
    std::string name;
    double freq;
    int mode;
    int speed_ms;
};

// ====================== 模式彻底修正版 ======================
const std::vector<NoteDef> NOTE_DICT = {
    // 低音区：模式 1 → 左 → 右
    {"C4", 261.63, 1, 140},
    {"D4", 293.66, 1, 130},
    {"E4", 329.63, 1, 120},
    {"F4", 349.23, 1, 110},
    {"G4", 392.00, 1, 100},
    {"A4", 440.00, 1,  90},
    {"B4", 493.88, 1,  80},

    // 中音区：模式 2 → 右 → 左
    {"C5", 523.25, 2, 140},
    {"D5", 587.33, 2, 130},
    {"E5", 659.25, 2, 120},
    {"F5", 698.46, 2, 110},
    {"G5", 783.99, 2, 100},
    {"A5", 880.00, 2,  90},
    {"B5", 987.77, 2,  80},

    // 高音区：模式 3 → 全体闪烁
    {"C6", 1046.50, 3, 140},
    {"D6", 1174.66, 3, 120},
    {"E6", 1318.51, 3, 100}
};
// ===========================================================

NoteDef identify_note(double freq) {
    for (const auto& note : NOTE_DICT) {
        if (std::abs(freq - note.freq) < note.freq * 0.10) {
            return note;
        }
    }
    return {"Unknown", 0.0, 0, 100};
}

int main() {
    system("pinctrl set 17,27,22,23,24,26 op");
    system("pinctrl set 26 dl");

    std::cout << "🌟 17键拇指琴灯光程序已启动\n" << std::endl;
    signal(SIGINT, signal_handler);

    std::thread anim_thread(led_animation_thread);

    snd_pcm_t *pcm_handle = nullptr;
    auto open_audio = [&]() {
        if (snd_pcm_open(&pcm_handle, DEVICE_NAME, SND_PCM_STREAM_CAPTURE, 0) < 0) return false;
        snd_pcm_hw_params_t *params;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_hw_params_any(pcm_handle, params);
        snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S32_LE);
        snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS);
        unsigned int rate = SAMPLE_RATE;
        snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
        snd_pcm_hw_params(pcm_handle, params);
        return true;
    };

    if (!open_audio()) {
        std::cerr << "❌ 无法打开麦克风！" << std::endl;
        running = false;
        anim_thread.join();
        return 1;
    }

    double *in = (double*)fftw_malloc(sizeof(double) * CHUNK_SIZE);
    fftw_complex *out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * (CHUNK_SIZE / 2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(CHUNK_SIZE, in, out, FFTW_MEASURE);
    std::vector<int32_t> buffer(CHUNK_SIZE * CHANNELS);

    int silence_counter = 0;

    while (running) {
        int err = snd_pcm_readi(pcm_handle, buffer.data(), CHUNK_SIZE);
        if (err < 0) {
            if (err == -EPIPE) snd_pcm_prepare(pcm_handle);
            else { snd_pcm_close(pcm_handle); std::this_thread::sleep_for(std::chrono::milliseconds(20)); open_audio(); }
            continue;
        }

        double sum_sq = 0;
        for (int i = 0; i < CHUNK_SIZE; i++) {
            double val = (double)buffer[i * CHANNELS] / 2147483648.0;
            sum_sq += val * val;
            in[i] = val;
        }
        double volume = std::sqrt(sum_sq / CHUNK_SIZE);

        if (volume > VOL_THRESHOLD) {
            silence_counter = 0;
            fftw_execute(plan);

            double max_mag = 0;
            int peak_idx = 0;
            for (int i = 0; i <= CHUNK_SIZE / 2; i++) {
                double freq = (double)i * SAMPLE_RATE / CHUNK_SIZE;
                if (freq < 200) continue;
                double mag = out[i][0]*out[i][0] + out[i][1]*out[i][1];
                if (mag > max_mag) { max_mag = mag; peak_idx = i; }
            }

            double peak_freq = (double)peak_idx * SAMPLE_RATE / CHUNK_SIZE;
            NoteDef detected = identify_note(peak_freq);

            if (detected.name != "Unknown") {
                anim_mode = detected.mode;
                anim_speed = detected.speed_ms;

                printf("\r🎹 %-3s %.1f Hz 模式:%d ", detected.name.c_str(), peak_freq, detected.mode);
                std::fflush(stdout);
            } else {
                printf("\r〰️ 未知音: %.1f Hz          ", peak_freq);
                std::fflush(stdout);
            }
        } else {
            silence_counter++;
            if (silence_counter > 6) {
                anim_mode = 0;
                printf("\r💤 安静，灯光已关闭          ");
                std::fflush(stdout);
                silence_counter = 6;
            }
        }
    }

    std::cout << "\n\n🛑 程序退出" << std::endl;
    anim_thread.join();
    if (pcm_handle) snd_pcm_close(pcm_handle);
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
    return 0;
}
