/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Jul-29-2026
 */

#include "app/app_main.h"

#include "cmsis_os.h"
#include "comm/uart/manager.h"
#include "stm32f4xx_hal_uart.h"

#include <cstdio>

extern "C" {
extern UART_HandleTypeDef huart1;
}

extern "C" void app_main(void)
{
    printf("\r\n=== STM32 UART1 Raw Test Started ===\r\n");
    
    uint8_t rx_byte = 0;

    while (true) {
        // Block until 1 byte arrives on USART1 (Beagle connection)
        if (HAL_UART_Receive(&huart1, &rx_byte, 1, HAL_MAX_DELAY) == HAL_OK) {
            // Print what was received out to USART2 (Your Laptop Terminal)
            printf("USART1 Rx Byte: 0x%02X ('%c')\r\n", rx_byte, rx_byte);
            HAL_UART_Transmit(&huart1, &rx_byte, 1, 100);
            // Toggle onboard Green LED for visual feedback
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        }
    }
}
