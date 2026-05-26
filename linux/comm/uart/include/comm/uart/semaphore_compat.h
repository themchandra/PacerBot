/**
 * @file semaphore_compat.h
 * @brief Lightweight counting semaphore compatibility layer for older libstdc++.
 * @author Hayden Mai
 * @date May-26-2026
 */

#ifndef COMM_UART_SEMAPHORE_COMPAT_H_
#define COMM_UART_SEMAPHORE_COMPAT_H_

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace uart {
    class CountingSemaphore {
      public:
        explicit CountingSemaphore(std::ptrdiff_t initialCount)
            : count_ {initialCount}
        {
        }

        void release()
        {
            {
                std::lock_guard<std::mutex> lock(mtx_);
                ++count_;
            }
            cv_.notify_one();
        }

        void acquire()
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait(lock, [this] { return count_ > 0; });
            --count_;
        }

      private:
        std::mutex mtx_;
        std::condition_variable cv_;
        std::ptrdiff_t count_;
    };
} // namespace uart

#endif