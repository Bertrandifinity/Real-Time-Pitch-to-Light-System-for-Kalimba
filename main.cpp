#include "AudioCapture.h"
#include "PitchAnalyzer.h"
#include "LedController.h"
#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> running(true);
void signal_handler(int signum) { running = false; }

int main() {
    std::cout << "🌟 Kalimba Lumina System Starting..." << std::endl;
    signal(SIGINT, signal_handler);

    AudioCapture audio;
    PitchAnalyzer analyzer;
    LedController leds;

    if (!audio.openDevice()) {
        std::cerr << "❌ Failed to open microphone!" << std::endl;
        return 1;
    }

    leds.start();
    double* audio_buffer = new double[CHUNK_SIZE];
    int silence_counter = 0;

    std::cout << "🎧 Listening..." << std::endl;

    while (running) {
        double volume = audio.readChunk(audio_buffer);

        if (volume > VOL_THRESHOLD) {
            silence_counter = 0;
            double peak_freq = 0.0;
            NoteDef note = analyzer.analyze(audio_buffer, peak_freq);

            if (note.name != "Unknown") {
                leds.updateState(note.mode, note.speed_ms);
                printf("\r🎹 Note: %-2s | Freq: %6.1f Hz   ", note.name.c_str(), peak_freq);
                std::fflush(stdout);
            }
        } else {
            silence_counter++;
            if (silence_counter > 5) {
                leds.updateState(0, 100); // Send silence state
                printf("\r💤 Silence...                      ");
                std::fflush(stdout);
                silence_counter = 5;
            }
        }
    }

    std::cout << "\n🛑 Shutting down safely..." << std::endl;
    leds.stop();
    delete[] audio_buffer;
    return 0;
}
