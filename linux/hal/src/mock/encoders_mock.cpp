#include "hal/encoders.h"
#include "hal/motors.h"
#include <iostream>
#include <cmath>


namespace hal::encoders {
    // Simulation parameters
    static constexpr float MAX_SPEED = 1.0f; // Max speed in m/s for simulation
    static const float ACCEL_RATE = 0.5f; // Acceleration rate in m/s^2
    static constexpr float FRICTION = 0.1f; // Slow the robot when there is no power

    // Simulation state
    static float simulated_speed = 0.0f; // Current speed in m/s

    float get_speed() {
        return simulated_speed;
    }

    void reset() {
        simulated_speed = 0.0f;
        std::cout << "MOCK: Encoders reset\n";
    }

    void update_simulation(float dt) {
        // Calculate target speed based on  motor duties
        float avg_duty = (hal::motors::current_left_duty + hal::motors::current_right_duty) / 2.0f;
        float target_speed = avg_duty * MAX_SPEED;

        // Calculate acceleration based on difference and direction
        float speed_diff = target_speed - simulated_speed;

        if (std::abs(speed_diff) < 0.01f) {
            // Close enough to target
            simulated_speed = target_speed;
        }   else if (speed_diff > 0) {
            // Speeding up
            simulated_speed += ACCEL_RATE * dt;
            if (simulated_speed > target_speed) {
                simulated_speed = target_speed;
            }
        } else {
            // Slowing down
            simulated_speed -= ACCEL_RATE * dt;
            if (simulated_speed < target_speed) {
                simulated_speed = target_speed;
            }
        }
        // Apply friction when motors are off
        if (avg_duty < 0.01f && simulated_speed > 0) {
            simulated_speed -= FRICTION * dt;
            if (simulated_speed < 0) 
                simulated_speed = 0;
        }
        std::cout << "MOCK: Simulated speed = " << simulated_speed << " m/s\n";
    }
}

