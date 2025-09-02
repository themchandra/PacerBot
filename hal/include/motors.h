#pragma once

namespace hal::motors {
    /**
     * Current duty cycles for left and right motors (0.0 to 1.0)
     * These are updated by set_duty and can be read by other modules
     */
     extern float current_left_duty;
     extern float current_right_duty;
    /**
     * Set the duty cycle for both motors
     * @param left Left motor duty cycle (0.0 to 1.0)
     * @param right Right motor duty cycle (0.0 to 1.0)
     */
    void set_duty(float left, float right);
}