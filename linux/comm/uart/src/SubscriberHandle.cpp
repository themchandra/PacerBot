/**
 * @file SubscriberHandle.cpp
 * @brief Handle to manage each subscriber packet queue
 * @author Hayden Mai
 * @date May-01-2026
 */

#include "comm/uart/SubscriberHandle.h"

namespace uart {
    SubscriberHandle::SubscriberHandle(std::initializer_list<ePacketID> ids_filter)
    {
        filter_.fill(false);

        // Filter to only push certain packets
        for (auto id : ids_filter) {
            filter_[static_cast<uint8_t>(id)] = true;
        }
    }


    void SubscriberHandle::push(const DataPacket &packet)
    {
        if (filter_[static_cast<uint8_t>(packet.getID())]) {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(packet);
            sem_.release();
        }
    }


    DataPacket SubscriberHandle::pop()
    {
        sem_.acquire(); // Blocking

        std::lock_guard<std::mutex> lock(mtx_);
        DataPacket packet = std::move(queue_.front());
        queue_.pop();
        return packet;
    }

} // namespace uart