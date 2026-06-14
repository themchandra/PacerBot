/**
 * @file control_loop.h
 * @brief Control loop that obtains new data & drives PIDController.
 * @author Hayden Mai
 * @date Jun-13-2026
 */

#ifndef APP_CTRL_LOOP_H_
#define APP_CTRL_LOOP_H_

#include "app/cmd_manager.h"
#include "app/pid_controller.h"
#include "hal/esc.h"
#include "hal/gps.h"
#include "hal/servo.h"

#include "cmsis_os.h"

namespace app {
    /**
     * @class ControlLoop
     * @brief Periodic control loop task that updates steering/speed control.
     *
     * This task owns ESC and servo outputs and is intended to run at a fixed
     * period under CMSIS-RTOS.
     */
    class ControlLoop {
      public:
        /**
         * @brief Construct a control loop and initialize output peripherals.
         * @param timer Timer handle used by both ESC and servo PWM outputs.
         * @param gps GPS module used to fetch ground speed in m/s.
         * @param cmd_manager Command manager used to fetch latest command values.
         */
        ControlLoop(TIM_HandleTypeDef *timer, hal::GPS &gps, CMDManager &cmd_manager);

        /**
         * @brief Start the control loop task.
         * @note If the task is already running, this call does nothing.
         */
        void start();

        /**
         * @brief Stop the control loop task and reset output/controller state.
         *
         * Stops task execution and performs output/controller cleanup so the
         * platform is left in a safe neutral state.
         */
        void stop();


      private:
        static constexpr uint32_t CONTROL_PERIOD_MS {100};

        // PID gains (tune these constants as needed)
        static constexpr float KP_SPEED {0.05f};
        static constexpr float KI_SPEED {0.0f};
        static constexpr float KD_SPEED {0.0f};

        static constexpr float KP_LINE {0.0f};
        static constexpr float KI_LINE {0.0f};
        static constexpr float KD_LINE {0.0f};

        // Normalized PID output range: -1.0 to +1.0
        static constexpr float PID_OUTPUT_MIN {-1.0f};
        static constexpr float PID_OUTPUT_MAX {1.0f};

        PIDController pid_speed_ {};
        PIDController pid_lines_ {};

        hal::ESC esc_;
        hal::Servo servo_;

        hal::GPS &gps_;
        CMDManager &cmd_manager_;

        static constexpr uint32_t STACK_SIZE_BYTES {1024};
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

        osThreadId_t taskHandle_ {};
        volatile bool isRunning_ {false};

        /**
         * @brief Static RTOS entrypoint that forwards execution to the instance loop.
         * @param args Pointer to ControlLoop instance.
         */
        static void threadTrampoline(void *args);

        /**
         * @brief Main periodic loop body for control updates.
         */
        void threadLoop();

        /**
         * @brief Convert a normalized PID output into an actuator pulse width.
         * @param normalized Normalized command in the range [-1.0, 1.0].
         * @param center_us Neutral pulse width.
         * @param min_us Minimum pulse width.
         * @param max_us Maximum pulse width.
         * @return Pulse width in microseconds.
         */
        static uint16_t normalizedToPulse(float normalized, uint16_t center_us,
                                          uint16_t min_us, uint16_t max_us);
    };
} // namespace app

#endif