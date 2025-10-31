/**
 * @file manager.h
 * @brief Manages UART communication between Radxa & STM32
 * @author Hayden Mai
 * @date Oct-29-2025
 */

#ifndef COMM_UART_MANAGER_H_
#define COMM_UART_MANAGER_H_

#include <cstdint>

/*
 * Additional features to add in the future:
 * - Queue size limit (prevent memory leak)
 * - Give uart::manager enqueue and dequeue wrapper functions
 * - Exceptions & exception handling
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

    void init();
    void deinit();

    // Threads management
    void start();
    void stop();
    eRunStatus isRunning();

} // namespace uart::manager

#endif