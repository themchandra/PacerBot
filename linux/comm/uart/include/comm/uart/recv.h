/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#ifndef COMM_UART_RECV_H_
#define COMM_UART_RECV_H_

#include "comm/uart/EventUART.h"
#include "comm/uart/packet_info.h"
#include "hal/SerialUART.h"

#include <memory>

namespace uart::recv {
    void init(std::shared_ptr<SerialUART> uartPtr);
    void deinit();

    // Thread management
    void start();
    void stop();
    bool isRunning();

    // Queue management
    std::optional<DataPacket> dequeue();
    size_t getQueueSize();
    bool isQueueEmpty();
    void clearQueue();

	// Event flag - Subscribes automatically by this module
    EventFlag &getEventFlag();

} // namespace uart::recv

#endif