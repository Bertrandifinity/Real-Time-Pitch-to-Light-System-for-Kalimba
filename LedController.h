#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <atomic>
#include <thread>

class LedController {
private:
    std::atomic<bool> running;
    std::atomic<int> anim_mode;
    std::atomic<int> anim_speed;
    std::thread worker;

    void animationLoop();

public:
    LedController();
    ~LedController();
    void start();
    void stop();
    void updateState(int mode, int speed_ms);
};

#endif
