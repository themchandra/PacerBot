/**
 * @file timing.cpp
 * @brief Manage time related processes
 * @author Hayden Mai
 * @date Oct-31-2025
 */

#define _POSIX_C_SOURCE 200809L

#include <cassert>
#include <chrono>
#include <thread>

#include "app/timing.h"

namespace {
    bool isInitialized_ {false};
}


namespace timing {
    void init(void)
    {
        assert(!isInitialized_);
        isInitialized_ = true;
    }


    void deinit(void)
    {
        assert(isInitialized_);
        isInitialized_ = false;
    }


    void sleepForMs(long long durationMs)
    {
        assert(isInitialized_);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }


    long long getTimeMs(void)
    {
        assert(isInitialized_);
        auto now      = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch());

        return duration.count();
    }

} // namespace timing