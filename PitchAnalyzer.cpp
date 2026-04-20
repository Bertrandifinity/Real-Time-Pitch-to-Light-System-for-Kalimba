#include "PitchAnalyzer.h"
#include <cmath>

PitchAnalyzer::PitchAnalyzer() {
    in = (double*) fftw_malloc(sizeof(double) * CHUNK_SIZE);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (CHUNK_SIZE / 2 + 1));
    plan = fftw_plan_dft_r2c_1d(CHUNK_SIZE, in, out, FFTW_MEASURE);
    initDictionary();
}

PitchAnalyzer::~PitchAnalyzer() {
    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);
}

void PitchAnalyzer::initDictionary() {
    dictionary = {
        {"C4", 261.6, 1, 150}, {"D4", 293.7, 1, 130}, {"E4", 329.6, 1, 110},
        {"F4", 349.2, 1, 90},  {"G4", 392.0, 1, 70},  {"A4", 440.0, 1, 50},  {"B4", 493.9, 1, 30},
        {"C5", 523.3, 2, 150}, {"D5", 587.3, 2, 130}, {"E5", 659.3, 2, 110},
        {"F5", 698.5, 2, 90},  {"G5", 784.0, 2, 70},  {"A5", 880.0, 2, 50},  {"B5", 987.8, 2, 30},
        {"C6", 1046.5, 3, 150},{"D6", 1174.7, 3, 130},{"E6", 1318.5, 3, 110},
        {"F6", 1396.9, 3, 90}, {"G6", 1568.0, 3, 70}, {"A6", 1760.0, 3, 50}, {"B6", 1975.5, 3, 30}
    };
}

NoteDef PitchAnalyzer::identifyNote(double freq) {
    for (const auto& note : dictionary) {
        if (std::abs(freq - note.freq) < (note.freq * 0.04)) {
            return note;
        }
    }
    return {"Unknown", 0.0, 0, 100};
}

NoteDef PitchAnalyzer::analyze(const double* input_buffer, double& out_peak_freq) {
    for (int i = 0; i < CHUNK_SIZE; i++) {
        in[i] = input_buffer[i];
    }
    
    fftw_execute(plan);
    
    double max_mag = 0;
    int peak_idx = 0;
    
    for (int i = 0; i <= CHUNK_SIZE / 2; i++) {
        double freq = (double)i * SAMPLE_RATE / CHUNK_SIZE;
        if (freq < 150) continue; 
        
        double mag = out[i][0]*out[i][0] + out[i][1]*out[i][1];
        if (mag > max_mag) { max_mag = mag; peak_idx = i; }
    }
    
    out_peak_freq = (double)peak_idx * SAMPLE_RATE / CHUNK_SIZE;
    return identifyNote(out_peak_freq);
}
