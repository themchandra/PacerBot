/**
 * @file send.cpp
 * @brief Handles data transmission via UART
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#include "comm/uart/config.h"
#include "comm/uart/send.h"

#include <atomic>
#include <cassert>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <semaphore>

// TODO: Semaphores for loop
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
	std::counting_semaphore<uart::config::MAX_TX_QUEUE_SIZE> queue_sem_(0);


    void serialAndSend()
    {
        uint8_t buffer[uart::config::READ_BUF_SIZE] {};

        // Serialize into the buffer
        size_t packetSize = queue_.front().serialize(buffer, sizeof(buffer));
        queue_.pop();

        uartPtr_->writeData(buffer, packetSize);
    }


    void thread_loop()
    {
        while (isThreadRunning_) {
            // Check if queue is empty:
            // - If empty, do nothing
            // - If not, serialize the first item in queue
            // 		- Make sure data serialization is valid
            //		- Send data via uart
			queue_sem_.acquire();
            std::lock_guard<std::mutex> lock(queue_mtx_);
            if (!queue_.empty()) {
                serialAndSend();
            }
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
        thread_.join();
    }


    bool isRunning()
    {
        assert(isInitialized_);
        return (isThreadRunning_);
    }


    void enqueue(DataPacket packet)
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

        // Create an empty queue and swap,
        // Destructor for DataPacket objects will run
        std::lock_guard<std::mutex> lock(queue_mtx_);
        std::queue<DataPacket> q_empty;
        std::swap(queue_, q_empty);
    }

} // namespace uart::send