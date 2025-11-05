/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-04-2025
 */

#ifndef COMM_UART_RECV_H_
#define COMM_UART_RECV_H_

#include "stm32f4xx_hal.h"

namespace uart::recv {
    void init(UART_HandleTypeDef *huart);
    void deinit();

    // Thread management
    /**
     * @brief Starts a task for parsing a buffer fed by DMA channel
     */
    void start();
    void stop();
    bool isRunning();

} // namespace uart::recv

#endif