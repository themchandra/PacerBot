#include "motors.h"
#include <iostream>

namespace hal::motors {
    void set_duty(float left, float right) {
        std::cout << "MOCK: Setting motors to left=" << left << ", right=" << right << std::endl;
    }
}