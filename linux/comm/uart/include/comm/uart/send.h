/**
 * @file send.h
 * @brief Handles data transmission via UART
 * @author Hayden Mai
 * @date Oct-17-2025
 */

#ifndef COMM_UART_SEND_H_
#define COMM_UART_SEND_H_

#include "comm/uart/packet_info.h"
#include "hal/SerialUART.h"

#include <memory>

namespace uart::send {
    void init(std::shared_ptr<SerialUART> uartPtr);
    void deinit();

    // Thread management
    void start();
    void stop();
    bool isRunning();

    // Queue management
    void enqueue(DataPacket packet);
    size_t getQueueSize();
    bool isQueueEmpty();
    void clearQueue();

} // namespace uart::send

#endif