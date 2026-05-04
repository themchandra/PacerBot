/**
 * @file recv.h
 * @brief Handles incoming packets from UART using DMA IDLE
 * @author Hayden Mai
 * @date May-03-2026
 */

#ifndef COMM_UART_RECV_H_
#define COMM_UART_RECV_H_

#include "cmsis_os.h"
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
    /**
     * @brief Dequeue a validated packet from the message queue.
     * @param packet Pointer to packet structure to fill.
     * @param timeout_ms Timeout in milliseconds (osWaitForever: wait indefinitely).
     * @return true if packet was dequeued, false otherwise.
     */
    bool dequeue(DataPacket_raw *packet, uint32_t timeout_ms = osWaitForever);
    bool isQueueEmpty();
    uint32_t getQueueCount();

} // namespace uart::recv

#endif