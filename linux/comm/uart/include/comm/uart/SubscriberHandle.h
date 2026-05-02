/**
 * @file SubscriberHandle.h
 * @brief Handle to manage each subscriber packet queue
 * @author Hayden Mai
 * @date May-01-2026
 */

#ifndef COMM_UART_SUBSCRIBER_HANDLE_H_
#define COMM_UART_SUBSCRIBER_HANDLE_H_

#include <array>
#include <mutex>
#include <queue>
#include <semaphore>

#include "comm/uart/packet_info.h"

namespace uart {
    class SubscriberHandle {
      public:
        SubscriberHandle(std::initializer_list<ePacketID> ids_filter);
        ~SubscriberHandle() = default;

        void push(const DataPacket &packet);
        DataPacket pop();

      private:
        static constexpr int MAX_COUNT {100};

        std::array<bool, static_cast<uint8_t>(ePacketID::TOTAL)> filter_;
        std::queue<DataPacket> queue_;
        std::mutex mtx_;
        std::counting_semaphore<MAX_COUNT> sem_ {0};
    };
} // namespace uart

#endif