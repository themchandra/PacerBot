/**
 * @file control_loop.cpp
 * @brief Control loop that drives PIDController.
 * @author Hayden Mai
 * @date May-14-2026
 */

#include "app/control_loop.h"

#include <stdint.h>

namespace app {

    ControlLoop::ControlLoop(TIM_HandleTypeDef *timer)
    {
        esc_.init(timer);
        servo_.init(timer);
    }


    void ControlLoop::start()
    {
        taskHandle_ = osThreadNew(threadTrampoline, this, &task_att_);
    }


    void ControlLoop::threadTrampoline(void *args)
    {
        ControlLoop *pThis = static_cast<ControlLoop *>(args);
        pThis->threadLoop();
    }


    void ControlLoop::threadLoop()
    {
        while (true) {
            // TODO: Obtain sensor data and update PID controllers
            // pid_speed_.update(current_speed, target_speed);
            // pid_lines_.update(line_position, center_line);

            // Get control outputs from PID controllers
            // esc_.set_pulse_us(pid_speed_.output);
            // servo_.set_pulse_us(pid_lines_.output);

            osDelay(10); // Control loop delay in milliseconds
        }
    }

} // namespace app