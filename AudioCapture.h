#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H

#include <alsa/asoundlib.h>
#include <vector>

class AudioCapture {
private:
    snd_pcm_t *pcm_handle;
    std::vector<int32_t> buffer;

public:
    AudioCapture();
    ~AudioCapture();
    bool openDevice();
    // Reads audio chunk and converts to normalized double array. Returns RMS volume.
    double readChunk(double* out_buffer); 
};

#endif
