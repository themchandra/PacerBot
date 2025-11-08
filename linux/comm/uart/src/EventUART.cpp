/**
 * @file EventUART.cpp
 * @brief Notify and waits for when there is a new message
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#include "comm/uart/EventUART.h"

namespace uart {
    void EventFlag::subscribe()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        num_subscribers_++;
    }


    void EventFlag::unsubscribe()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (num_subscribers_ > 0) {
            num_subscribers_--;
        }
    }


    void EventFlag::notify(eEvent event)
    {
        // Scope so that lock is released afterwards
        {
            std::lock_guard<std::mutex> lock(mtx_);
            event_     = event;
            ack_count_ = 0;
        }

        cv_.notify_all(); // Wake all subscribers
    }


    eEvent EventFlag::wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this] { return event_ != eEvent::NONE; });

        ack_count_++;
        eEvent newEvent = event_;
        if (ack_count_ >= num_subscribers_) {
            event_ = eEvent::NONE;
        }

        return newEvent;
    }


    void EventFlag::reset()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        event_     = eEvent::NONE;
        ack_count_ = 0;
    }

} // namespace uart