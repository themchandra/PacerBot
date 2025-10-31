#include "state_machine.h"
#include "hal/motors.h"
#include "hal/encoders.h"

namespace app {
    // Constants
    constexpr float MOTOR_STOP_DUTY = 0.0f;
    constexpr float FIXED_SPEED_DUTY = 0.25f; // Fixed duty for milestone 1
    
    // Current operating mode of the robot
    static Mode current_mode = Mode::IDLE;

    // Target speed in meters per second
    static float target_speed_mps = 0.0f;

    void set_target_speed(float mps) {
        target_speed_mps = mps;
        if (current_mode != Mode::E_STOP) {
            if (mps != 0.0f) {
                current_mode = Mode::RUN;
            } else {
                current_mode = Mode::IDLE;
            }
        }
    }

    void tick(float dt) {
        // Update encoders simulation (only in mock implementation)
        hal::encoders::update_simulation(dt);
        
        switch (current_mode) {
            case Mode::IDLE:
                hal::motors::set_duty(MOTOR_STOP_DUTY, MOTOR_STOP_DUTY);
                break;

            case Mode::RUN:
                // Using fixed speed for milestone 1
                // Will implement variable speed based on target_speed_mps in future milestones
                hal::motors::set_duty(FIXED_SPEED_DUTY, FIXED_SPEED_DUTY);
                break;

            case Mode::E_STOP:
                hal::motors::set_duty(MOTOR_STOP_DUTY, MOTOR_STOP_DUTY);
                break;
        }
    }

    void emergency_stop() {
        current_mode = Mode::E_STOP;
        hal::motors::set_duty(MOTOR_STOP_DUTY, MOTOR_STOP_DUTY);
    }
    
    Mode mode() {
        return current_mode;
    }

    void reset() {
        if (current_mode == Mode::E_STOP) {
            current_mode = Mode::IDLE;
            target_speed_mps = 0.0f;
        }
    }
}