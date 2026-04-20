#include "LedController.h"
#include <cstdlib>
#include <string>
#include <chrono>

LedController::LedController() : running(false), anim_mode(0), anim_speed(100) {
    system("pinctrl set 17,27,22,23,24,26 op");
    system("pinctrl set 26 dl");
}

LedController::~LedController() {
    stop();
}

void LedController::start() {
    running = true;
    worker = std::thread(&LedController::animationLoop, this);
}

void LedController::stop() {
    running = false;
    if (worker.joinable()) {
        worker.join();
    }
    system("pinctrl set 17,27,22,23,24 dl > /dev/null 2>&1");
}

void LedController::updateState(int mode, int speed_ms) {
    anim_mode.store(mode);
    anim_speed.store(speed_ms);
}

void LedController::animationLoop() {
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
            for(int i = 0; i < 5; i++) {
                if (i == step) dh_pins += std::to_string(leds[i]) + ",";
                else dl_pins += std::to_string(leds[i]) + ",";
            }
            step = (step + 1) % 5;
        } else if (mode == 2) { 
            for(int i = 0; i < 5; i++) {
                if (4 - i == step) dh_pins += std::to_string(leds[i]) + ",";
                else dl_pins += std::to_string(leds[i]) + ",";
            }
            step = (step + 1) % 5;
        } else if (mode == 3) { 
            if (step % 2 == 0) dh_pins = "17,27,22,23,24,";
            else dl_pins = "17,27,22,23,24,";
            step = (step + 1) % 2;
        }

        if (!dh_pins.empty()) dh_pins.pop_back();
        if (!dl_pins.empty()) dl_pins.pop_back();

        std::string cmd = "";
        if (!dh_pins.empty()) cmd += "pinctrl set " + dh_pins + " dh > /dev/null 2>&1 ; ";
        if (!dl_pins.empty()) cmd += "pinctrl set " + dl_pins + " dl > /dev/null 2>&1";

        system(cmd.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(speed));
    }
}
