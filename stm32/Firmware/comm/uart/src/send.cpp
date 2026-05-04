/**
 * @file send.cpp
 * @brief Handles data transmission via UART DMA
 * @author Hayden Mai
 * @date May-03-2026
 */

#include "comm/uart/send.h"

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace {
    bool isInitialized_ {false};
    UART_HandleTypeDef *huart_;

    constexpr uint32_t TX_COMPLETE_FLAG {0x01};

    // Send Management
    osMessageQueueId_t packetQueue_;
    // Dropped packet counter for observability
    uint32_t droppedPackets_ {};

    // Task definition
    std::atomic_bool isTaskRunning_ {false};
    osThreadId_t taskHandle_;
    constexpr uint32_t STACK_SIZE_BYTES {512};

    // Statically allocate the stack to avoid kernel heap allocation failures on
    // targets with small RTOS heaps (G4 devices sometimes have smaller default
    // osHeap memory compared to F4 Nucleo boards)
    StaticTask_t task_cb_;
    StackType_t task_stack_mem_[STACK_SIZE_BYTES];

    constexpr osThreadAttr_t task_att_ = {
        .name       = "sendTask",
        .attr_bits  = 0,
        .cb_mem     = &task_cb_,
        .cb_size    = sizeof(task_cb_),
        .stack_mem  = task_stack_mem_,
        .stack_size = sizeof(task_stack_mem_),
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0,
    };


    void threadLoop(void *argument)
    {
        (void)argument;

        while (isTaskRunning_) {
            uart::DataPacket_raw sendPacket {};

            // Blocking call, waiting until a packet is available
            if (osMessageQueueGet(packetQueue_, &sendPacket, nullptr, osWaitForever)
                != osOK) {
                continue;
            }

            // Transmit data
            const HAL_StatusTypeDef ret = HAL_UART_Transmit_DMA(
                huart_, reinterpret_cast<uint8_t *>(&sendPacket), sendPacket.totalSize());
            if (ret != HAL_OK) {
                printf("HAL_UART_Transmit_DMA failed: %d\n\r", static_cast<int>(ret));
                continue;
            }

            // Wait for the DMA completion callback
            const uint32_t flags
                = osThreadFlagsWait(TX_COMPLETE_FLAG, osFlagsWaitAny, osWaitForever);
            if (flags != TX_COMPLETE_FLAG) {
                printf("UART TX wait failed: 0x%08lx\n\r",
                       static_cast<unsigned long>(flags));
            }
        }
    }


} // namespace


namespace uart::send {
    void init(UART_HandleTypeDef *huart)
    {
        assert(!isInitialized_);
        huart_       = huart;
        packetQueue_ = osMessageQueueNew(MAX_QUEUE_SIZE, sizeof(DataPacket_raw), NULL);
        if (packetQueue_ == nullptr) {
            printf("Failed to create message queue\n\r");
            return;
        }
        isInitialized_ = true;
    }


    void deinit()
    {
        assert(isInitialized_);
        osMessageQueueDelete(packetQueue_);
        isInitialized_ = false;
    }


    void start()
    {
        assert(isInitialized_);
        isTaskRunning_ = true;
        taskHandle_    = osThreadNew(threadLoop, NULL, &task_att_);
        if (taskHandle_ == nullptr) {
            isTaskRunning_ = false;
            printf("Failed to create send task (osThreadNew returned NULL)\n\r");
        }
    }


    void stop()
    {
        assert(isInitialized_);
        isTaskRunning_ = false;
        osThreadTerminate(taskHandle_);
        taskHandle_ = nullptr;
    }


    bool isRunning()
    {
        assert(isInitialized_);
        return isTaskRunning_;
    }


    void handleTxComplete(UART_HandleTypeDef *huart)
    {
        if (huart == huart_ && taskHandle_ != nullptr) {
            (void)osThreadFlagsSet(taskHandle_, TX_COMPLETE_FLAG);
        }
    }


    static void enqueue_packet(uart::ePacketID id, const void *payload, uint8_t len)
    {
        uart::DataPacket_raw packet {};
        packet.sync   = SYNC_SEND;
        packet.id     = id;
        packet.length = len;

        if (payload != nullptr && len > 0) {
            std::memcpy(packet.data, payload, len);
        }

        packet.data[packet.length] = calculate_crc8(
            reinterpret_cast<const uint8_t *>(&packet), packet.totalSize() - 1);

        // Try non-blocking enqueue. If full, drop the oldest packet and retry.
        const osStatus_t status = osMessageQueuePut(packetQueue_, &packet, 0, 0);
        if (status == osOK) {
            return;
        }

        if (status == osErrorResource) {
            // Drop oldest and retry once
            uart::DataPacket_raw discarded {};
            const bool dropped
                = (osMessageQueueGet(packetQueue_, &discarded, NULL, 0) == osOK);
            if (dropped) {
                const osStatus_t retryStatus
                    = osMessageQueuePut(packetQueue_, &packet, 0, 0);
                if (retryStatus == osOK) {
                    droppedPackets_++;
                    printf("Queue full, dropped oldest (dropped=%lu)\n\r",
                           droppedPackets_);
                    return;
                }
            }
        }

        // Final fallback: report failure
        printf("Failed to queue packet (status=%d)\n\r", status);
    }


    void enqueue_imu(IMU_data data)
    {
        assert(isInitialized_);
        enqueue_packet(ePacketID::TELEM_IMU, &data, sizeof(IMU_data));
    }


    void enqueue_ack()
    {
        assert(isInitialized_);
        enqueue_packet(ePacketID::STM32_ACK, nullptr, 0);
    }


    void enqueue_debug(const char *msg)
    {
        assert(isInitialized_);

        if (msg == nullptr) {
            return;
        }

        // Cut off string if too long
        constexpr uint8_t MAX_MSG_SIZE {DATA_MAX_SIZE - 1};
        uint8_t len = static_cast<uint8_t>(strlen(msg));
        if (len > MAX_MSG_SIZE) {
            len = MAX_MSG_SIZE;
        }

        enqueue_packet(ePacketID::STM32_DEBUG, msg, len);
    }


    bool isQueueEmpty()
    {
        assert(isInitialized_);
        return osMessageQueueGetCount(packetQueue_) == 0;
    }


    uint32_t getQueueCount()
    {
        assert(isInitialized_);
        return osMessageQueueGetCount(packetQueue_);
    }

} // namespace uart::send