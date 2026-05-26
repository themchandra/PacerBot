/**
 * @file send.cpp
 * @brief Handles data transmission via UART
 * @author Hayden Mai
 * @date May-04-2026
 */

#include "comm/uart/send.h"
#include "comm/uart/config.h"

#include <atomic>
#include <cassert>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "comm/uart/semaphore_compat.h"

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
    uart::CountingSemaphore queue_sem_ {0};


    void thread_loop()
    {
        uint8_t buffer[uart::config::READ_BUF_SIZE] {};

        while (isThreadRunning_) {
            queue_sem_.acquire();

            if (!isThreadRunning_) {
                break; // Exit loop by stop()
            }

            size_t packetSize {};
            {
                std::lock_guard<std::mutex> lock(queue_mtx_);
                if (queue_.empty()) {
                    continue; // End loop iteration early
                }

                // Serialize inside the lock (fast operation)
                packetSize = queue_.front().serialize(buffer, sizeof(buffer));
                queue_.pop();
            }

            // Write outside the lock to reduce contention
            uartPtr_->writeData(buffer, packetSize);
        }
    }

} // namespace

namespace uart::send {
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
        queue_sem_.release(); // acquire() unblocks to end loop
        thread_.join();
    }


    bool isRunning()
    {
        assert(isInitialized_);
        return (isThreadRunning_);
    }


    void enqueue(DataPacket &&packet)
    {
        assert(isInitialized_);
        std::lock_guard<std::mutex> lock(queue_mtx_);
        queue_.push(std::move(packet));
        queue_sem_.release();
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
        std::lock_guard<std::mutex> lock(queue_mtx_);
        queue_ = {}; // Swap queue with empty one
    }

} // namespace uart::send