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
     * @param buffer Packet data to validate against.
     * @param startIdx Buffer index of the candidate sync byte.
     * @param dataLen Remaining bytes available from startIdx.
     * @return -1 if invalid, 0 if the packet is incomplete, or the packet
     *         length if the packet is valid.
     */
    int validateData(const uint8_t *buffer, size_t startIdx, size_t dataLen)
    {
        // Need at least header (3 bytes: sync, id, len)
        if (dataLen < uart::HEADER_SIZE) {
            return 0; // Incomplete
        }

        // Validate sync byte - accept both SYNC_RECV (0x5A) and SYNC_SEND (0xA5)
        // for loopback testing. deserialize() will enforce SYNC_RECV for actual STM32
        // packets.
        const uint8_t syncByte = buffer[startIdx];
        if (syncByte != uart::SYNC_RECV && syncByte != uart::SYNC_SEND) {
            return -1; // Invalid
        }


        // Validate packet ID is in valid range
        const auto packetId = static_cast<uart::ePacketID>(buffer[startIdx + 1]);
        if (packetId > uart::ePacketID::STM32_DEBUG) {
            return -1; // Invalid packet ID
        }

        // Extract and validate payload length
        const uint8_t payloadLen = buffer[startIdx + 2];
        if (payloadLen >= uart::DATA_MAX_SIZE) {
            return -1; // Payload length out of bounds
        }

        const size_t packetLen = uart::HEADER_SIZE + payloadLen + uart::CRC_SIZE;

        // Check if we have the complete packet
        if (dataLen < packetLen) {
            return 0; // Incomplete
        }

        // Return packet length on success
        return static_cast<int>(packetLen);
    }

    void parseBuffer()
    {
        if (streamBuffer_.empty()) {
            return;
        }

        size_t processedIdx {0};
        bool keepParsing {true};

        while (keepParsing) {
            // Search for sync byte starting from processedIdx offset
            auto syncByteIter = std::find(streamBuffer_.begin() + processedIdx,
                                          streamBuffer_.end(), uart::SYNC_RECV);
            if (syncByteIter == streamBuffer_.end()) {
                // No more sync bytes found, discard all processed junk
                processedIdx = streamBuffer_.size();
                keepParsing  = false;
                continue;
            }

            // Calculate index of sync byte relative to buffer start
            size_t syncIdx      = std::distance(streamBuffer_.begin(), syncByteIter);
            size_t remainingLen = streamBuffer_.size() - syncIdx;

            // Validate packet from the sync byte
            int validationResult
                = validateData(streamBuffer_.data(), syncIdx, remainingLen);

            if (validationResult > 0) {
                // Valid packet - deserialize and process
                size_t packetLen = static_cast<size_t>(validationResult);
                auto packet      = uart::DataPacket::deserialize(
                    streamBuffer_.data() + syncIdx, packetLen);
                if (packet.has_value()) {
                    publish(packet.value());
                    processedIdx = syncIdx + packetLen;
                } else {
                    // Deserialize failed (CRC error) - skip this bad sync byte
                    processedIdx = syncIdx + 1;
                }
            } else if (validationResult == 0) {
                // Incomplete packet - wait for more data
                processedIdx = syncIdx; // Keep unprocessed data
                keepParsing  = false;
            } else {
                // Invalid packet, skip bad sync byte
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
        std::array<uint8_t, uart::config::READ_BUF_SIZE> readBuf;
        while (isThreadRunning_) {
            ssize_t bytesRead = uartPtr_->readData(readBuf.data(), readBuf.size());
            if (bytesRead > 0) {
                streamBuffer_.insert(streamBuffer_.end(), readBuf.begin(),
                                     readBuf.begin() + bytesRead);
                parseBuffer();
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
        stop();
        {
            std::lock_guard<std::mutex> lock(sub_mtx_);
            subscribers_.clear();
        }
        streamBuffer_.clear();

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
        if (thread_.joinable()) {
            thread_.join();
        }
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