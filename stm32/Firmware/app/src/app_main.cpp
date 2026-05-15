/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date May-14-2026
 */

#include "app/app_main.h"
#include "app/cmd_manager.h"
#include "app/control_loop.h"
#include "app/imu_task.h"
#include "comm/uart/manager.h"

#include "cmsis_os.h"
#include "main.h"

#include <cstdio>

// External global variables from Core/Src/main.c
// NOTE: Only pass them as reference through initialization for modules/classes,
// 		 save them as a pointer for use.
extern "C" {
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern TIM_HandleTypeDef htim3;
}

// IMU I2C device address
constexpr int IMU_ADDRESS = 0x68;

extern "C" void app_main(void)
{
    // Initialize UART for command reception
    uart::manager::init(&huart1, uart::manager::eUARTInstance::UART_1);
    uart::manager::start();

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
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        osDelay(4900);
    }
}