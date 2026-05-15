/**
 * @file control_loop.h
 * @brief Control loop that obtains new data & drives PIDController.
 * @author Hayden Mai
 * @date May-14-2026
 */

#ifndef APP_CTRL_LOOP_H_
#define APP_CTRL_LOOP_H_

#include "app/pid_controller.h"
#include "hal/esc.h"
#include "hal/servo.h"

#include "cmsis_os.h"

namespace app {
    class ControlLoop {
      public:
        ControlLoop(TIM_HandleTypeDef *timer);
        void start();


      private:
        PIDController pid_speed_ {};
        PIDController pid_lines_ {};

        hal::ESC esc_;
        hal::Servo servo_;

        static constexpr uint32_t STACK_SIZE_BYTES {512};
        static constexpr osThreadAttr_t task_att_ {
            .name       = "ControlLoopTask",
            .attr_bits  = 0,
            .cb_mem     = nullptr,
            .cb_size    = 0,
            .stack_mem  = nullptr,
            .stack_size = STACK_SIZE_BYTES,
            .priority   = osPriorityNormal,
            .tz_module  = 0,
            .reserved   = 0,
        };
        osThreadId_t taskHandle_;

        static void threadTrampoline(void *args);
        void threadLoop();
    };
} // namespace app

#endif