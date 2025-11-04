/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Nov-04-2025
 */

#include "app/app_main.h"
#include "comm/uart/recv.h"

#include "cmsis_os.h"
#include "main.h"

// External global variables from Core/Src/main.c
extern "C" {
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
}


extern "C" void app_main(void)
{
    // initialize other modules and start new threads and stuff
	// call C++ functions here
    while (true) {
        HAL_UART_Transmit(&huart2, (uint8_t *)"Hello\r\n", 7, 100);
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        osDelay(1000);
    }

    return;
}