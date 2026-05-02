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
#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>


namespace {
    bool isInitialized_ {false};

    // Shared pointer to the serial port
    std::shared_ptr<SerialUART> uartPtr_ {nullptr};

    // Manage a list of subscribers
    std::vector<std::shared_ptr<uart::SubscriberHandle>> subscribers_;
    std::mutex sub_mtx_; // Used for managing subscriber subscriptions

    // Buffer for storing partial packets (deque for O(1) front/back erasure)
    std::deque<uint8_t> streamBuffer_;

    // Threading
    std::atomic_bool isThreadRunning_ {false};
    std::thread thread_;


    void publish(const uart::DataPacket &packet)
    {
        std::lock_guard<std::mutex> lock(sub_mtx_);

        // Distribute packet to each subscriber, each has their own internal filter
        for (auto &subscriber : subscribers_) {
            subscriber->push(packet);
        }
    }


    void parseNQueue(const uint8_t *rawData, size_t data_len)
    {
        if (!rawData || data_len == 0) {
            return;
        }

        streamBuffer_.insert(streamBuffer_.end(), rawData, rawData + data_len);

        while (true) {
            // Search for sync byte in the buffer
            auto syncByteIter
                = std::find(streamBuffer_.begin(), streamBuffer_.end(), uart::SYNC_RECV);
            if (syncByteIter == streamBuffer_.end()) {
                // No sync byte found, clear buffer and wait for more data
                streamBuffer_.clear();
                return;
            }

            // Remove any data before the sync byte
            if (syncByteIter != streamBuffer_.begin()) {
                streamBuffer_.erase(streamBuffer_.begin(), syncByteIter);
            }

            // Verify we have at least the header (sync + id + length)
            constexpr size_t HEADER_SIZE {3};
            if (streamBuffer_.size() < HEADER_SIZE) {
                return;
            }

            // Extract payload length from header (byte index 2)
            const uint8_t payloadLen {streamBuffer_[2]};
            const size_t packetLen {HEADER_SIZE + payloadLen + 1};

            // Verify we have the complete packet
            if (streamBuffer_.size() < packetLen) {
                // Incomplete packet, wait for more data
                return;
            }

            // Attempt to deserialize the packet
            // Convert to vector for deserialization (required for contiguous memory)
            std::vector<uint8_t> packetData(streamBuffer_.begin(),
                                            streamBuffer_.begin() + packetLen);
            auto packet = uart::DataPacket::deserialize(packetData.data(), packetLen);
            if (packet.has_value()) {
                // Valid packet - publish to all subscribers
                publish(packet.value());

                // Remove processed packet from buffer
                streamBuffer_.erase(streamBuffer_.begin(),
                                    streamBuffer_.begin() + packetLen);
            } else {
                // Invalid packet - discard first byte and retry
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
            uint8_t read_buf[uart::config::READ_BUF_SIZE] {};
            size_t bytesRead = uartPtr_->readData(read_buf, sizeof(read_buf));

            if (bytesRead > 0) {
                parseNQueue(read_buf, bytesRead);
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
        auto subscriber = std::make_shared<SubscriberHandle>(ids_filter);

        std::lock_guard<std::mutex> lock(sub_mtx_);
        subscribers_.push_back(subscriber);
        return subscriber;
    }


    void unsubscribe(std::shared_ptr<SubscriberHandle> subscriber)
    {
        assert(isInitialized_);

        std::lock_guard<std::mutex> lock(sub_mtx_);
        std::erase(subscribers_, subscriber);
    }

} // namespace uart::recv