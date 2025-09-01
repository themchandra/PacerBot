#pragma once

namespace hal::motors {
    /**
     * Set the duty cycle for both motors
     * @param left Left motor duty cycle (0.0 to 1.0)
     * @param right Right motor duty cycle (0.0 to 1.0)
     */
    void set_duty(float left, float right);
}