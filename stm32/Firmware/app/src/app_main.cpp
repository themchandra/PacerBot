/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Jun-13-2026
 */

#include "app/app_main.h"
#include "app/cmd_manager.h"
#include "app/control_loop.h"
#include "app/imu_task.h"
#include "comm/uart/manager.h"

#include "cmsis_os.h"
#include "hal/esc.h"
#include "hal/imu.h"
#include "hal/servo.h"
#include "hal/ultrasonic.h"
#include "main.h"
#include "stm32f4xx_hal_tim.h"

#include <cstdio>

// External global variables from Core/Src/main.c
// NOTE: Only pass them as reference through initialization for modules/classes,
// 		 save them as a pointer for use.
extern "C" {
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern TIM_HandleTypeDef htim3;
}

// Forward HAL timer input-capture callback into the Ultrasonic class
extern "C" void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1 && hal::Ultrasonic::instance) {
        hal::Ultrasonic::instance->handle_capture_callback();
    }
}

extern "C" void app_main(void)
{
    hal::Ultrasonic HC(&htim1, TIM_CHANNEL_1, GPIOB, GPIO_PIN_10);
    // Hardware test for ESC at 1550 us and servo to the right
    static hal::ESC esc;
    static hal::Servo servo;
    const bool escOk   = esc.init(&htim3);
    const bool servoOk = servo.init(&htim3);

    // Create and start command manager
    app::CMDManager cmd_manager;
    cmd_manager.start();

    // Create and start IMU task
    app::IMUTask imu_task(&hi2c1, IMU_ADDRESS);
    imu_task.start();

    // Create and start control loop
    app::ControlLoop control_loop(&htim3, imu_task, cmd_manager);
    control_loop.start();

    // Main idle loop - flash LED briefly every 5 seconds
    while (true) {
        // small ultrasonic test
        HC.trigger();
        osDelay(60);

        float distance = HC.get_distance_cm();
        printf("Distance = %.3f cm\n", distance);

        servo.set_pulse_us(hal::Servo::MIN_US); // Keep servo to the right

        esc.set_pulse_us(1425); // Backward
        osDelay(PWM_TEST_DELAY_MS);
        esc.set_pulse_us(1575); // Forward

        if (ENABLE_UART_DEBUG) {
            static uint32_t debugCounter {1};
            char debugMessage[32] {};
            std::snprintf(debugMessage, sizeof(debugMessage), "Debug here! #%lu",
                          static_cast<unsigned long>(debugCounter++));
            uart::send::enqueue_debug(debugMessage);
            uart::send::enqueue_ack();
        }

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        osDelay(PWM_TEST_DELAY_MS);
    }
}