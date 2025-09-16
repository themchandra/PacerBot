#include "motors.h"
#include <iostream>

namespace hal::motors {
    float current_left_duty = 0.0f;
    float current_right_duty = 0.0f;

    void set_duty(float left, float right) {

        current_left_duty = left;
        current_right_duty = right;
        
        std::cout << "MOCK: Setting motors to left=" << left << ", right=" << right << std::endl;
    }
}