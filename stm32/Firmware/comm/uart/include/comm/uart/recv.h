/**
 * @file recv.h
 * @brief Handles incoming packets from UART using DMA IDLE
 * @author Hayden Mai
 * @date Dec-16-2025
 */

#ifndef COMM_UART_RECV_H_
#define COMM_UART_RECV_H_

#include "comm/uart/packet_info.h"
#include "stm32f4xx_hal.h"

namespace uart::recv {
    // Queue size limit
    constexpr int MAX_QUEUE_SIZE {5};

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
    bool dequeue(DataPacket_raw *packet, uint32_t timeout_ms);
    bool isQueueEmpty();
    uint32_t getQueueCount();

} // namespace uart::recv

#endif