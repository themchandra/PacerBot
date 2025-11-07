/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-06-2025
 */

#ifndef COMM_UART_RECV_H_
#define COMM_UART_RECV_H_

#include "stm32f4xx_hal.h"

namespace uart::recv {
    // Queue size limit
    constexpr int MAX_QUEUE_SIZE {5};

    enum class eFlags : uint32_t {
        CALLBACK = (1 << 0),
        CMD      = (1 << 1),
        CONF     = (1 << 2),
        RADXA    = (1 << 3),
        // Limit is (1 << 32)
    };

    void init(UART_HandleTypeDef *huart);
    void deinit();

    // Thread management
    /**
     * @brief Starts a task for parsing a buffer fed by DMA channel
     */
    void start();
    void stop();
    bool isRunning();

    /**
     * @brief Update parsing index tracking & trigger CALLBACK flag
     */
    void updateBufInd(uint16_t index);

    // Publisher Queue management
    osEventFlagsId_t getEventFlag();
    bool dequeue(DataPacket_raw *packet);
	bool isQueueEmpty();
	uint32_t getQueueCount();

} // namespace uart::recv

#endif