/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date May-01-2026
 */

#include "comm/uart/recv.h"
#include "comm/uart/config.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>


namespace {
    bool isInitialized_ {false};

    // Shared pointer to the serial port
    std::shared_ptr<SerialUART> uartPtr_ {nullptr};

    // Manage a list of subscribers
    std::vector<std::shared_ptr<uart::SubscriberHandle>> handles_;
    std::mutex handle_mtx_; // Used for managing handle subscriptions

    // Buffer for storing partial packets
    std::vector<uint8_t> streamBuffer_;

    // Threading
    std::atomic_bool isThreadRunning_ {false};
    std::thread thread_;


    void publish(const uart::DataPacket &packet)
    {
        std::lock_guard<std::mutex> lock(handle_mtx_);

        // Distribute packet to each handle, each has their own internal filter
        for (auto &handle : handles_) {
            handle->push(packet);
        }
    }


    void parseNQueue(const uint8_t *data, size_t len)
    {
        if (!data || len == 0) {
            return;
        }

        streamBuffer_.insert(streamBuffer_.end(), data, data + len);

        while (true) {
            auto syncIt
                = std::find(streamBuffer_.begin(), streamBuffer_.end(), uart::SYNC_RECV);
            if (syncIt == streamBuffer_.end()) {
                streamBuffer_.clear();
                return;
            }

            if (syncIt != streamBuffer_.begin()) {
                streamBuffer_.erase(streamBuffer_.begin(), syncIt);
            }

            constexpr size_t HEADER_SIZE {3};
            if (streamBuffer_.size() < HEADER_SIZE) {
                return;
            }

            const uint8_t payloadLen {streamBuffer_[2]};
            const size_t packetLen {HEADER_SIZE + payloadLen + 1};

            if (streamBuffer_.size() < packetLen) {
                return;
            }

            auto packet = uart::DataPacket::deserialize(streamBuffer_.data(), packetLen);
            if (packet.has_value()) {
                publish(packet.value());
                streamBuffer_.erase(streamBuffer_.begin(),
                                    streamBuffer_.begin() + packetLen);
            } else {
                streamBuffer_.erase(streamBuffer_.begin());
            }
        }
    }


    void thread_loop()
    {
        while (isThreadRunning_) {
            // When a message arrives
            //	- Parse the data (find the sync byte)
            //	- Deserialize into DataPacket
            // 	- Save into the queue
            uint8_t buffer[uart::config::READ_BUF_SIZE] {};
            size_t bytesRead = uartPtr_->readData(buffer, sizeof(buffer));

            if (bytesRead > 0) {
                parseNQueue(buffer, bytesRead);
            }
        }
    }

} // namespace


namespace uart::recv {
    void init(std::shared_ptr<SerialUART> uartPtr)
    {
        assert(!isInitialized_);

        // Share ownership of pointer
        uartPtr_ = uartPtr;
        assert(uartPtr_ != nullptr);

        isInitialized_ = true;
    }


    void deinit()
    {
        assert(isInitialized_);

        // Releases ownership of object
        uartPtr_.reset();

        isInitialized_ = false;
    }


    void start()
    {
        assert(isInitialized_);
        isThreadRunning_ = true;
        thread_          = std::thread(thread_loop);
    }


    void stop()
    {
        assert(isInitialized_);
        isThreadRunning_ = false;
        thread_.join();
    }


    bool isRunning()
    {
        assert(isInitialized_);
        return (isThreadRunning_);
    }


    std::shared_ptr<SubscriberHandle>
    subscribe(std::initializer_list<ePacketID> ids_filter)
    {
        assert(isInitialized_);
        auto handle = std::make_shared<SubscriberHandle>(ids_filter);

        std::lock_guard<std::mutex> lock(handle_mtx_);
        handles_.push_back(handle);
        return handle;
    }


    void unsubscribe(std::shared_ptr<SubscriberHandle> handle)
    {
        assert(isInitialized_);

        std::lock_guard<std::mutex> lock(handle_mtx_);
        std::erase(handles_, handle);
    }

} // namespace uart::recv