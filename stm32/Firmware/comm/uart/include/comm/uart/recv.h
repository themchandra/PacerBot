/**
 * @file recv.h
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-05-2025
 */

#ifndef COMM_UART_RECV_H_
#define COMM_UART_RECV_H_

#include "stm32f4xx_hal.h"

namespace uart::recv {
	enum class eFlags : uint32_t {
		CALLBACK = 0x01,
		CMD,
		CFG,
		RADXA,
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

	// Setters for callbacks
	/**
	 * @brief Update parsing index tracking & trigger CALLBACK flag
	 */
	void updateBufInd(uint16_t index);

	// Queue management

} // namespace uart::recv

#endif