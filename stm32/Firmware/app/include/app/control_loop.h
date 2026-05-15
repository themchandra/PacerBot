/**
 * @file control_loop.h
 * @brief Control loop that obtains new data & drives PIDController.
 * @author Hayden Mai
 * @date May-14-2026
 */

#ifndef APP_CTRL_LOOP_H_
#define APP_CTRL_LOOP_H_

#include "app/cmd_manager.h"
#include "app/imu_task.h"
#include "app/pid_controller.h"
#include "hal/esc.h"
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
         * @param imu_task IMU task used to fetch latest inertial measurements.
         * @param cmd_manager Command manager used to fetch latest command values.
         */
        ControlLoop(TIM_HandleTypeDef *timer, IMUTask &imu_task, CMDManager &cmd_manager);

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
        PIDController pid_speed_ {};
        PIDController pid_lines_ {};

        hal::ESC esc_;
        hal::Servo servo_;

        IMUTask &imu_task_;
        CMDManager &cmd_manager_;

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
    };
} // namespace app

#endif