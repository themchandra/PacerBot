/**
 * @file manager.h
 * @brief Manages UART communication between Host & STM32
 * @author Hayden Mai
 * @date May-04-2026
 */

#ifndef COMM_UART_MANAGER_H_
#define COMM_UART_MANAGER_H_

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
    enum class eUARTInstance : uint8_t {
        UART_1,
        UART_2,
    };

    enum class eRunStatus {
        RUNNING,
        RECV_STOPPED,
        SEND_STOPPED,
        BOTH_STOPPED,
    };

    void init(UART_HandleTypeDef *huart, eUARTInstance instance);
    void deinit();

    // Threads management
    void start();
    void stop();
    eRunStatus isRunning();

} // namespace uart::manager

#endif