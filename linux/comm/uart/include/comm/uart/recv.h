/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date May-01-2026
 */

#ifndef COMM_UART_RECV_H_
#define COMM_UART_RECV_H_

#include "comm/uart/SubscriberHandle.h"
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

    std::shared_ptr<SubscriberHandle>
    subscribe(std::initializer_list<ePacketID> ids_filter);
    void unsubscribe(std::shared_ptr<SubscriberHandle> subscriber);

} // namespace uart::recv

#endif