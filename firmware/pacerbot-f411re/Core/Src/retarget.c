
/**
 ******************************************************************************
 * @file    uart_redirect.c
 * @brief   Retargets printf() output to UART using HAL.
 *  * retarget.c
 *
 *  Created on: Oct 8, 2025
 *      Author: michaelchandra
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

#include "stm32f4xx_hal.h"
#include <sys/unistd.h>         // STDOUT_FILENO, STDERR_FILENO

extern UART_HandleTypeDef huart2;	// UART handle defined in main.c


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
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, HAL_MAX_DELAY);
}

int _write(int file, char *data, int len)
{
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        for (int i = 0; i < len; ++i) {
            if (data[i] == '\n') {
                const char crlf[2] = {'\r','\n'};
                tx_blocking((const uint8_t*)crlf, 2);
            } else {
                tx_blocking((uint8_t const *)&data[i], 1);
            }
        }
        return len;
    }
    return 0;
}


