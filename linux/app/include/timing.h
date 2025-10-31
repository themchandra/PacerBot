/**
 * @file timing.h
 * @brief Manage time related processes
 * @author Hayden Mai
 * @date Oct-31-2025
 */

#ifndef APP_TIMING_H_
#define APP_TIMING_H_

namespace timing {
    void init(void);
    void deinit(void);

    void sleepForMs(long long durationMs);
    long long getTimeMs(void);

} // namespace timing

#endif