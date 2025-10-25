/*
 * uart_redirect.c
 *
 *  Created on: Oct 9, 2025
 *      Author: michaelchandra
 */

/**
 ******************************************************************************
 * @file    uart_redirect.c
 * @brief   Retargets printf() output to UART using HAL.
 *
 * This file implements the _write() syscall, allowing standard I/O functions
 * such as printf() or puts() to send data over UART.
 *
 * Example usage:
 *     printf("Hello, world!\n");
 *
 * This will transmit "Hello, world!\r\n" through huart2.
 ******************************************************************************
 */

#include "stm32f4xx_hal.h"   // Adjust for your MCU family (e.g. stm32f1xx_hal.h)
#include <sys/unistd.h>       // Provides STDOUT_FILENO, STDERR_FILENO

extern UART_HandleTypeDef huart2;  // UART handle defined in main.c or elsewhere

/**
 * @brief  Transmit data over UART in blocking mode.
 * @param  buf: Pointer to the data buffer to send
 * @param  len: Number of bytes to send
 *
 * This helper function calls HAL_UART_Transmit() with HAL_MAX_DELAY
 * to ensure that transmission completes before returning.
 */
static void tx_blocking(const uint8_t *buf, uint16_t len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

/**
 * @brief  Retargets the C library printf() function to UART.
 * @param  file: File descriptor (STDOUT or STDERR supported)
 * @param  data: Pointer to the data buffer to write
 * @param  len:  Length of the data buffer
 * @retval Number of bytes transmitted, or 0 on error
 *
 * This function is called internally whenever printf(), puts(), or
 * other standard output functions are used. It checks whether the
 * file descriptor corresponds to STDOUT or STDERR and transmits
 * each character over UART.
 *
 * For terminal compatibility, '\n' is automatically converted to "\r\n".
 */
int _write(int file, char *data, int len)
{
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        for (int i = 0; i < len; ++i) {
            if (data[i] == '\n') {
                const char crlf[2] = {'\r', '\n'};
                tx_blocking((const uint8_t *)crlf, 2);
            } else {
                tx_blocking((const uint8_t *)&data[i], 1);
            }
        }
        return len;
    }

    // Return 0 if file descriptor is unsupported
    return 0;
}



