/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date May-04-2026
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


namespace {
    bool isInitialized_ {false};

    // Shared pointer to the serial port
    std::shared_ptr<SerialUART> uartPtr_ {nullptr};

    // Manage a list of subscribers
    std::vector<std::shared_ptr<uart::SubscriberHandle>> subscribers_;
    std::mutex sub_mtx_; // Used for managing subscriber subscriptions

    // Buffer for storing partial packets
    std::vector<uint8_t> streamBuffer_;

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


    /**
     * @brief Check whether a packet candidate is valid.
     * @param startIdx Buffer index of the candidate sync byte.
     * @param dataLen Remaining bytes available from startIdx.
     * @return -2 if invalid, -1 if the packet is incomplete, or the packet
     *         length if the packet is valid.
     */
    int validateData(size_t startIdx, size_t dataLen)
    {
        // Need at least header (3 bytes: sync, id, len)
        if (dataLen < uart::HEADER_SIZE) {
            return -1; // Incomplete
        }

        // Validate sync byte
        const uint8_t syncByte = streamBuffer_[startIdx];
        if (syncByte != uart::SYNC_RECV) {
            return -2; // Invalid
        }

        // Validate packet ID is in valid range
        const auto packetId = static_cast<uart::ePacketID>(streamBuffer_[startIdx + 1]);
        if (packetId >= uart::ePacketID::TOTAL) {
            return -2; // Invalid packet ID
        }

        // Extract and validate payload length
        const uint8_t payloadLen = streamBuffer_[startIdx + 2];
        if (payloadLen >= uart::DATA_MAX_SIZE) {
            return -2; // Payload length out of bounds
        }

        // Validate special rule: zero-length packets must be ACK packets
        if (payloadLen == 0) {
            if (packetId != uart::ePacketID::RAD_ACK
                && packetId != uart::ePacketID::STM32_ACK) {
                return -2; // Invalid zero-length non-ACK packet
            }
        }

        const size_t packetLen = uart::HEADER_SIZE + payloadLen + uart::CRC_SIZE;

        // Check if we have the complete packet
        if (dataLen < packetLen) {
            return 1; // Incomplete
        }

        // Return packet length on success
        return static_cast<int>(packetLen);
    }

    void parseBuffer()
    {
        if (streamBuffer_.empty()) {
            return;
        }

        size_t processedIdx{0};

        while (true) {
            // Search for sync byte starting from processedBytes offset
            auto syncByteIter = std::find(streamBuffer_.begin() + processedIdx,
                                          streamBuffer_.end(), uart::SYNC_RECV);
            if (syncByteIter == streamBuffer_.end()) {
                // No more sync bytes found, discard all processed junk
                processedIdx = streamBuffer_.size();
                break;
            }

            // Calculate index of sync byte relative to buffer start
            size_t syncIdx      = std::distance(streamBuffer_.begin(), syncByteIter);
            size_t remainingLen = streamBuffer_.size() - syncIdx;

            // Validate packet from the sync byte 
            int validationResult = validateData(syncIdx, remainingLen);

            if (validationResult >= 0) {
                // Valid packet - deserialize and process
                size_t packetLen = static_cast<size_t>(validationResult);
                auto packet      = uart::DataPacket::deserialize(
                    streamBuffer_.data() + syncIdx, packetLen);
                if (packet.has_value()) {
                    // Valid packet - publish to all subscribers
                    publish(packet.value());
                    processedIdx = syncIdx + packetLen;
                } else {
                    // Deserialize failed (CRC error) - skip this bad sync byte
                    processedIdx = syncIdx + 1;
                }
            } else if (validationResult == -1) {
                // Incomplete packet - wait for more data
                processedIdx = syncIdx; // Keep unprocessed data
                break;
            } else {
                // Invalid packet (validationResult == -1) - skip bad sync byte
                processedIdx = syncIdx + 1;
            }
        }

        // Remove all processed bytes in buffer
        if (processedIdx > 0) {
            streamBuffer_.erase(streamBuffer_.begin(),
                                streamBuffer_.begin() + processedIdx);
        }
    }


    void thread_loop()
    {
        while (isThreadRunning_) {
            const size_t oldSize {streamBuffer_.size()};

            streamBuffer_.resize(oldSize + uart::config::READ_BUF_SIZE);

            ssize_t bytesRead = uartPtr_->readData(streamBuffer_.data() + oldSize,
                                                   uart::config::READ_BUF_SIZE);
            if (bytesRead > 0) {
                streamBuffer_.resize(oldSize + static_cast<size_t>(bytesRead));
                parseBuffer();
            } else {
                streamBuffer_.resize(oldSize);
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