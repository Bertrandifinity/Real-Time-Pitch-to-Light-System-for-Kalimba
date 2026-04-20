#include "AudioCapture.h"
#include "Config.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

AudioCapture::AudioCapture() : pcm_handle(nullptr) {
    buffer.resize(CHUNK_SIZE * CHANNELS);
}

AudioCapture::~AudioCapture() {
    if (pcm_handle) {
        snd_pcm_close(pcm_handle);
    }
}

bool AudioCapture::openDevice() {
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
}

double AudioCapture::readChunk(double* out_buffer) {
    int err = snd_pcm_readi(pcm_handle, buffer.data(), CHUNK_SIZE);
    if (err < 0) {
        if (err == -EPIPE) snd_pcm_prepare(pcm_handle);
        else {
            snd_pcm_close(pcm_handle);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            openDevice();
        }
        return 0.0; // Return zero volume on error
    }

    double sum_sq = 0;
    for (int i = 0; i < CHUNK_SIZE; i++) {
        double val = (double)buffer[i * CHANNELS] / 2147483648.0;
        sum_sq += val * val;
        out_buffer[i] = val;
    }
    return std::sqrt(sum_sq / CHUNK_SIZE); // Return RMS volume
}
