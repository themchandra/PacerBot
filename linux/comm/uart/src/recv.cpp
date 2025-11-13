/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#include "comm/uart/config.h"
#include "comm/uart/recv.h"

#include <atomic>
#include <cassert>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

namespace {
    bool isInitialized_ {false};

    // Shared pointer to the serial port
    std::shared_ptr<SerialUART> uartPtr_ {nullptr};

    // Queue for storing messages
    std::queue<uart::DataPacket> queue_;

    // Threading
    std::atomic_bool isThreadRunning_ {false};
    std::thread thread_;
    std::mutex queue_mtx_;

    // Event handling
    uart::EventFlag eventFlag_ {};


    void parseNQueue(uint8_t *data, size_t len)
    {
        // TODO: Determine if this is correct in case where the stream is
        // continuous? Might need to do a robust implementation if so...
        // - Add current stream of bytes into buffer
        // - Parse buffer:
        //		- If not in a middle of a byte, begin parse with sync byte, loop
        //		  until no more sync bytes are found
        //		- If already in a middle of a byte, continue where it was left
        //		  off, loop until no more sync bytes are found
        auto packet = uart::DataPacket::deserialize(data, len);
        if (packet.has_value()) {
            std::lock_guard<std::mutex> lock(queue_mtx_);
            queue_.push(packet.value());

            // Determine event for other threads
            uart::eEvent event {uart::eEvent::NONE};

            switch (packet.value().getID()) {
            case uart::ePacketID::TELEM_IMU:
            case uart::ePacketID::TELEM_ULT:
            case uart::ePacketID::TELEM_ENC:
            case uart::ePacketID::TELEM_PID:
            case uart::ePacketID::TELEM_BATTERY:
                event = uart::eEvent::TELEMETRY;
                break;
            case uart::ePacketID::STM32_STATUS:
                event = uart::eEvent::STATUS;
                break;
            case uart::ePacketID::STM32_ACK:
                event = uart::eEvent::ACK;
                break;
            case uart::ePacketID::STM32_DEBUG:
                event = uart::eEvent::DEBUG;
                break;
            default:
                event = uart::eEvent::NONE;
                break;
            }
            eventFlag_.notify(event);
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


    std::optional<DataPacket> dequeue()
    {
        assert(isInitialized_);

        std::lock_guard<std::mutex> lock(queue_mtx_);
        if (queue_.empty()) {
            return std::nullopt; // no packets
        }

        DataPacket packet = std::move(queue_.front());
        queue_.pop();
        return packet;
    }


    size_t getQueueSize()
    {
        assert(isInitialized_);
        std::lock_guard<std::mutex> lock(queue_mtx_);
        return queue_.size();
    }


    bool isQueueEmpty()
    {
        assert(isInitialized_);
        std::lock_guard<std::mutex> lock(queue_mtx_);
        return queue_.empty();
    }


    void clearQueue()
    {
        assert(isInitialized_);

        // Create an empty queue and swap,
        // Destructor for DataPacket objects will run
        std::lock_guard<std::mutex> lock(queue_mtx_);
        std::queue<DataPacket> q_empty;
        std::swap(queue_, q_empty);
    }


    EventFlag &getEventFlag()
    {
        assert(isInitialized_);
        eventFlag_.subscribe();
        return eventFlag_;
    }

} // namespace uart::recv