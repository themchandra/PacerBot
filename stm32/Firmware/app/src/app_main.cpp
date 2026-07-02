/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Jul-02-2026
 */

#include "app/app_main.h"
#include "app/cmd_manager.h"
#include "app/control_loop.h"

#include "comm/uart/callbacks.h"
#include "comm/uart/manager.h"

#include "hal/gps.h"

#include "cmsis_os.h"
#include "main.h"

#include <cstdio>

// External global variables from Core/Src/main.c
// NOTE: Only pass them as reference through initialization for modules/classes,
// 		 save them as a pointer for use.
//       DO NOT use 'extern' anywhere else outside of this file!!
extern "C" {
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern TIM_HandleTypeDef htim3;
}

extern "C" void app_main(void)
{
    // Manual control toggle
    constexpr bool MAN_CTL_MODE {true};

    // Peripherals & Hardware sensors
    uart::manager::init(&huart1, uart::manager::eUARTInstance::UART_1);

    // App layer inits
    app::ControlLoop control_loop(&htim3, &huart6);
    app::CMDManager cmd_manager(control_loop);

    // Start tasks loops
    uart::manager::start();
    cmd_manager.start();
    control_loop.start();

    control_loop.set_target_speed(2.0f);
    osDelay(15000);
    //control_loop.set_target_speed(20.0f);
    //osDelay(10000);
    control_loop.set_target_speed(0.0f);

    // Main 'idle' loop
    while (true) {
        // Sign-of-life LED Blinking
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        osDelay(100);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        osDelay(4900);
    }
}
