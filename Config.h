#ifndef CONFIG_H
#define CONFIG_H
#include <string>

// System configurations
#define DEVICE_NAME "plughw:2,0" 
#define SAMPLE_RATE 48000
#define CHUNK_SIZE 4096      
#define CHANNELS 2
#define VOL_THRESHOLD 0.0005 

// Data structure for musical notes
struct NoteDef {
    std::string name;
    double freq;
    int mode;       // 1: L-to-R, 2: R-to-L, 3: Flash
    int speed_ms;
};

#endif // CONFIG_H
