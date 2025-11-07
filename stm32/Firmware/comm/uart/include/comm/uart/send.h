/**
 * @file send.h
 * @brief Handles data transmission via UART DMA
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#ifndef COMM_UART_SEND_H_
#define COMM_UART_SEND_H_

#include "comm/uart/packet_info.h"

#include "stm32f4xx_hal.h"

namespace uart::send {
    // TODO: Use HAL's structures instead
    struct IMU_data {
        int16_t accel_x {};
        int16_t accel_y {};
        int16_t accel_z {};
        int16_t gyro_x {};
        int16_t gyro_y {};
        int16_t gyro_z {};
    };

    constexpr int MAX_QUEUE_SIZE {5};

    void init(UART_HandleTypeDef *huart);
    void deinit();

    // Thread management
    void start();
    void stop();
    bool isRunning();

    // Enqueuing data
    void enqueue_IMU(IMU_data data);
    // void enqueue_Ultrasonic(uint32_t data);
    // void enqueue_Encoder(Encoder_data data);
    // void enqueue_Battery(Battery_data data);
    // void enqueue_PID(PID_data data);
    // void enqueue_status(Status_data data);
    void enqueue_ack();
    void enqueue_debug(const char *msg); // String limit of 100

    // Queue management
    bool isQueueEmpty();
    uint32_t getQueueCount();


} // namespace uart::send

#endif