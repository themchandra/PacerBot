/**
 * @file manager.h
 * @brief Manages UART communication between Radxa & STM32
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#ifndef COMM_UART_MANAGER_H_
#define COMM_UART_MANAGER_H_

#include "stm32f4xx_hal.h"
#include "comm/uart/recv.h"
#include "comm/uart/send.h"

/*
 * Additional features to add in the future:
 * - Give uart::manager enqueue and dequeue wrapper functions (maybe?)
 */

/**
 * @namespace uart::manager
 * @brief Manages UART communication interface.
 *
 * This module initializes/deinitializes and start/stop both send and recv modules. To
 * send and receive message from the uart port, use send::enqueue() and recv::dequeue()
 * functions respectively.
 */
namespace uart::manager {
    enum class eRunStatus {
        RUNNING,
        RECV_STOPPED,
        SEND_STOPPED,
        BOTH_STOPPED,
    };

    void init(UART_HandleTypeDef *huart);
    void deinit();

    // Threads management
    void start();
    void stop();
    eRunStatus isRunning();

} // namespace uart::manager

#endif