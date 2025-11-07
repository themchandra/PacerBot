/**
 * @file send.cpp
 * @brief Handles data transmission via UART DMA
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#include "comm/uart/packet_info.h"
#include "comm/uart/send.h"

#include "cmsis_os.h"

#include <atomic>
#include <cassert>
#include <cstring>

namespace {
    bool isInitialized_ {false};
    UART_HandleTypeDef *huart_;

    // Threading
    std::atomic_bool isTaskRunning_ {false};
    osSemaphoreId_t semTaskLoop_;

    // Send Management
    osMessageQueueId_t packetQueue_;
    osSemaphoreId_t semQueue_;

    // Task definition
    osThreadId_t taskHandle_;
    constexpr uint32_t STACK_SIZE_BYTES {512};
    constexpr osThreadAttr_t task_att_ {
        .name       = "sendTask",
        .attr_bits  = 0,
        .cb_mem     = nullptr,
        .cb_size    = 0,
        .stack_mem  = nullptr,
        .stack_size = STACK_SIZE_BYTES,
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0,
    };

    void threadLoop(void *argument)
    {
        if (argument) {
        }

        while (isTaskRunning_) {
            osSemaphoreAcquire(semQueue_, osWaitForever);

            uart::DataPacket_raw sendPacket {};
            osMessageQueueGet(packetQueue_, &sendPacket, 0, 0);
            HAL_UART_Transmit_DMA(huart_, (uint8_t *)&sendPacket, sendPacket.totalSize());

            // TODO: Make constants and do automatic calculate with baudrate for allow
            // 		 time for DMA transmission
            osDelay(10); // Maximum size DataPacket_raw is 104 bytes
        }
    }


} // namespace


namespace uart::send {
    void init(UART_HandleTypeDef *huart)
    {
        assert(!isInitialized_);
        huart_         = huart;
        packetQueue_   = osMessageQueueNew(MAX_QUEUE_SIZE, sizeof(DataPacket_raw), NULL);
        semTaskLoop_   = osSemaphoreNew(1, 0, NULL);
        semQueue_      = osSemaphoreNew(MAX_QUEUE_SIZE, 0, NULL);
        isInitialized_ = true;
    }


    void deinit()
    {
        assert(isInitialized_);
        osSemaphoreDelete(semTaskLoop_);
        osSemaphoreDelete(semQueue_);
        osMessageQueueDelete(packetQueue_);
        isInitialized_ = false;
    }


    // Thread management
    void start()
    {
        assert(isInitialized_);
        taskHandle_    = osThreadNew(threadLoop, NULL, &task_att_);
        isTaskRunning_ = true;
    }


    void stop()
    {
        assert(isInitialized_);
        isTaskRunning_ = false;

        osSemaphoreAcquire(semTaskLoop_, osWaitForever);
    }


    bool isRunning()
    {
        assert(isInitialized_);
        return isTaskRunning_;
    }


    // Enqueuing data
    void enqueue_IMU(IMU_data data)
    {
        assert(isInitialized_);

        DataPacket_raw packet {};
        packet.sync   = SYNC_SEND;
        packet.id     = ePacketID::TELEM_IMU;
        packet.length = sizeof(IMU_data);
        std::memcpy(packet.data, &data, sizeof(IMU_data));
        packet.data[packet.length]
            = calculate_crc8((uint8_t *)&packet, packet.totalSize() - 1);

        osMessageQueuePut(packetQueue_, &packet, 0, 0);
        osSemaphoreRelease(semQueue_);
    }


    void enqueue_ack()
    {
        assert(isInitialized_);

        DataPacket_raw packet {};
        packet.sync = SYNC_SEND;
        packet.id   = ePacketID::STM32_ACK;
        packet.data[packet.length]
            = calculate_crc8((uint8_t *)&packet, packet.totalSize() - 1);

        osMessageQueuePut(packetQueue_, &packet, 0, 0);
        osSemaphoreRelease(semQueue_);
    }


    void enqueue_debug(const char *msg)
    {
        assert(isInitialized_);

        DataPacket_raw packet {};
        packet.sync   = SYNC_SEND;
        packet.id     = ePacketID::STM32_DEBUG;
        packet.length = strlen(msg);
        std::memcpy(packet.data, msg, packet.length);
        packet.data[packet.length]
            = calculate_crc8((uint8_t *)&packet, packet.totalSize() - 1);

        osMessageQueuePut(packetQueue_, &packet, 0, 0);
        osSemaphoreRelease(semQueue_);
    }

    bool isQueueEmpty()
    {
        assert(isInitialized_);
        if (osMessageQueueGetCount(packetQueue_) == 0) {
            return true;
        }
        return false;
    }


    uint32_t getQueueCount()
    {
        assert(isInitialized_);
        return osMessageQueueGetCount(packetQueue_);
    }

} // namespace uart::send