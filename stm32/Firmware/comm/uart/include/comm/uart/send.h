/**
 * @file send.h
 * @brief Handles data transmission via UART DMA
 * @author Hayden Mai
 * @date Nov-06-2025
 */

#ifndef COMM_UART_SEND_H_
#define COMM_UART_SEND_H_

#include "comm/uart/packet_info.h"

#include "stm32f4xx_hal.h"

namespace uart::send {
    struct IMU_data {
        int16_t accel_x {};
        int16_t accel_y {};
        int16_t accel_z {};
        int16_t gyro_x {};
        int16_t gyro_y {};
        int16_t gyro_z {};
    };

	void init(UART_HandleTypeDef *huart);
	void deinit();

} // namespace uart::send

#endif