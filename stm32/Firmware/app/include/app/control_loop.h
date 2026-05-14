/**
 * @file control_loop.h
 * @brief Control loop that obtains new data & drives PIDController.
 * @author Hayden Mai
 * @date May-13-2026
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
        ControlLoop();
        void start();


      private:
        PIDController pid_speed_ {};
        PIDController pid_lines_ {};

        hal::ESC esc_;
        hal::Servo servo_;

        osthreadId_t taskHandle_;
        void threadLoop(void *args);
    };
} // namespace app

#endif