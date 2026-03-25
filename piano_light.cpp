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

// --- Audio & Hardware Configuration ---
#define DEVICE_NAME "plughw:2,0" 
#define SAMPLE_RATE 48000
#define CHUNK_SIZE 4096      
#define CHANNELS 2
#define VOL_THRESHOLD 0.0005 

std::atomic<bool> running(true);

// --- Animation Control Variables (Shared between threads) ---
std::atomic<int> anim_mode(0);   // 0: All Off, 1: L-to-R Marquee, 2: R-to-L Marquee, 3: Flash
std::atomic<int> anim_speed(100); // Animation interval in milliseconds

// Signal handler for safe shutdown (Ctrl+C)
void signal_handler(int signum) { running = false; }

// --- Independent LED Animation Thread ---
// Runs continuously in the background to render smooth light effects 
// without blocking the main audio processing thread.
void led_animation_thread() {
    int leds[5] = {17, 27, 22, 23, 24};
    int step = 0;
    
    // Initialize all LEDs to OFF (Drive Low)
    system("pinctrl set 17,27,22,23,24 dl > /dev/null 2>&1");

    while (running) {
        int mode = anim_mode.load();
        int speed = anim_speed.load();

        if (mode == 0) {
            // Silence mode: All LEDs OFF
            system("pinctrl set 17,27,22,23,24 dl > /dev/null 2>&1");
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            step = 0; // Reset animation step
            continue;
        }

        std::string cmd = "pinctrl set ";
        
        if (mode == 1) { 
            // Mode 1: Left-to-Right Marquee
            for(int i = 0; i < 5; i++) {
                if (i == step) cmd += std::to_string(leds[i]) + " dh ";
                else cmd += std::to_string(leds[i]) + " dl ";
            }
            step = (step + 1) % 5;
            
        } else if (mode == 2) { 
            // Mode 2: Right-to-Left Marquee
            for(int i = 0; i < 5; i++) {
                if (4 - i == step) cmd += std::to_string(leds[i]) + " dh ";
                else cmd += std::to_string(leds[i]) + " dl ";
            }
            step = (step + 1) % 5;
            
        } else if (mode == 3) { 
            // Mode 3: Collective Flashing (All LEDs blink synchronously)
            if (step % 2 == 0) cmd += "17,27,22,23,24 dh ";
            else cmd += "17,27,22,23,24 dl ";
            step = (step + 1) % 2;
        }

        cmd += "> /dev/null 2>&1";
        system(cmd.c_str()); // Execute GPIO pin change
        
        std::this_thread::sleep_for(std::chrono::milliseconds(speed));
    }
    
    // Ensure LEDs are turned off before thread exits
    system("pinctrl set 17,27,22,23,24 dl > /dev/null 2>&1");
}

// --- 21-Note Dictionary Definition ---
struct NoteDef {
    std::string name;
    double freq;
    int mode;       // 1: Left-to-Right, 2: Right-to-Left, 3: Flash
    int speed_ms;   // Lower value means faster animation
};

const std::vector<NoteDef> NOTE_DICT = {
    // Low Octave (C4-B4) -> Mode 1: Left to Right, speed increases with pitch
    {"C4", 261.6, 1, 150}, {"D4", 293.7, 1, 130}, {"E4", 329.6, 1, 110},
    {"F4", 349.2, 1, 90},  {"G4", 392.0, 1, 70},  {"A4", 440.0, 1, 50},  {"B4", 493.9, 1, 30},
    // Middle Octave (C5-B5) -> Mode 2: Right to Left, speed increases with pitch
    {"C5", 523.3, 2, 150}, {"D5", 587.3, 2, 130}, {"E5", 659.3, 2, 110},
    {"F5", 698.5, 2, 90},  {"G5", 784.0, 2, 70},  {"A5", 880.0, 2, 50},  {"B5", 987.8, 2, 30},
    // High Octave (C6-B6) -> Mode 3: Flash, speed increases with pitch
    {"C6", 1046.5, 3, 150},{"D6", 1174.7, 3, 130},{"E6", 1318.5, 3, 110},
    {"F6", 1396.9, 3, 90}, {"G6", 1568.0, 3, 70}, {"A6", 1760.0, 3, 50}, {"B6", 1975.5, 3, 30}
};

// Function to match captured frequency to the closest musical note
NoteDef identify_note(double freq) {
    for (const auto& note : NOTE_DICT) {
        // Allow a slight error margin (approx. half a semitone)
        if (std::abs(freq - note.freq) < (note.freq * 0.04)) {
            return note;
        }
    }
    return {"Unknown", 0.0, 0, 100};
}

int main() {
    // Set all LED pins and Buzzer(26) to Output Mode (op) and turn off Buzzer
    system("pinctrl set 17,27,22,23,24,26 op");
    system("pinctrl set 26 dl");

    std::cout << "🌟 21-Key Full Range Light Show Active!\n"
              << "🎧 Listening to C4 (261Hz) - B6 (1975Hz)...\n" << std::endl;
    signal(SIGINT, signal_handler);

    // Launch background LED animation thread
    std::thread anim_thread(led_animation_thread);

    snd_pcm_t *pcm_handle = nullptr;
    
    // Lambda function to initialize the ALSA audio capture device
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
        std::cerr << "❌ Error: Cannot open microphone device!" << std::endl;
        running = false;
        anim_thread.join();
        return 1;
    }

    // Allocate memory for Fast Fourier Transform (FFT)
    double *in = (double*) fftw_malloc(sizeof(double) * CHUNK_SIZE);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (CHUNK_SIZE / 2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(CHUNK_SIZE, in, out, FFTW_MEASURE);
    std::vector<int32_t> buffer(CHUNK_SIZE * CHANNELS);

    int silence_counter = 0;

    // Main Audio Processing Loop
    while (running) {
        int err = snd_pcm_readi(pcm_handle, buffer.data(), CHUNK_SIZE);
        
        // Handle I2S bus connection drops seamlessly
        if (err < 0) {
            if (err == -EPIPE) snd_pcm_prepare(pcm_handle);
            else { 
                snd_pcm_close(pcm_handle); 
                std::this_thread::sleep_for(std::chrono::milliseconds(20)); 
                open_audio(); 
            }
            continue;
        }

        // Calculate RMS Volume
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
            
            // Search for the strongest frequency (ignoring < 200Hz noise)
            for (int i = 0; i <= CHUNK_SIZE / 2; i++) {
                double freq = (double)i * SAMPLE_RATE / CHUNK_SIZE;
                if (freq < 200) continue; 
                double mag = out[i][0]*out[i][0] + out[i][1]*out[i][1];
                if (mag > max_mag) { max_mag = mag; peak_idx = i; }
            }
            
            double peak_freq = (double)peak_idx * SAMPLE_RATE / CHUNK_SIZE;
            NoteDef detected = identify_note(peak_freq);

            if (detected.name != "Unknown") {
                // Instantly update the animation thread's behavior
                anim_mode.store(detected.mode);
                anim_speed.store(detected.speed_ms);
                
                printf("\r🎹 Note: %-2s | Freq: %6.1f Hz | Mode: %d | Speed: %3d ms   ", 
                       detected.name.c_str(), peak_freq, detected.mode, detected.speed_ms);
                std::fflush(stdout);
            } else {
                printf("\r〰️ Unknown Pitch: %6.1f Hz                                  ", peak_freq);
                std::fflush(stdout);
            }
        } else {
            silence_counter++;
            // If environment is quiet for approx. 0.3 seconds, turn off LEDs
            if (silence_counter > 5) { 
                anim_mode.store(0); // Send OFF command to animation thread
                printf("\r💤 Silence... LEDs reset.                               ");
                std::fflush(stdout);
                silence_counter = 5;
            }
        }
    }

    std::cout << "\n\n🛑 Shutting down system securely..." << std::endl;
    anim_thread.join(); // Wait for animation thread to finish cleanly
    
    // Free allocated memory and close audio handler
    if (pcm_handle) snd_pcm_close(pcm_handle);
    fftw_destroy_plan(plan);
    fftw_free(in); fftw_free(out);
    return 0;
}