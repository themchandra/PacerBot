/**
 * @file uart_comm.h
 * @brief Manages UART communication between Radxa & STM32
 * @author Hayden Mai
 * @date Oct-08-2025
 */

#ifndef UART_COMM_H_
#define UART_COMM_H_

#include <cstdint>

namespace UART_Comm {
    /** @brief Max message size for queuing & receiving commands */
    const uint8_t MAX_MSG_SIZE {128};

    /** @brief Message structure */
    struct Message {
        uint8_t data[MAX_MSG_SIZE];
        uint8_t len
    };

    void init();
    void cleanup();

    /**
     * @brief Starts thread to receive data from UART
     */
    void recv_start();
    void recv_stop();

    /**
     * @brief Starts thread to transmit data in queue via UART.
     */
    void send_start();
    void send_stop();

    void enqueue_command(struct Message &msg);
    void

} // namespace UART_Comm

#endif