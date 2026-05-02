/**
 * @file send.cpp
 * @brief Handles data transmission via UART DMA
 * @author Hayden Mai
 * @date May-01-2026
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

    // Task definition
    std::atomic_bool isTaskRunning_ {false};
    osThreadId_t taskHandle_;
    constexpr uint32_t STACK_SIZE_BYTES {512};

    // Statically allocate the stack to avoid kernel heap allocation failures on
    // targets with small RTOS heaps (G4 devices sometimes have smaller default
    // osHeap memory compared to F4 Nucleo boards)
    StaticTask_t task_cb_;
    StackType_t task_stack_mem_[STACK_SIZE_BYTES];

    static osThreadAttr_t task_att_ = {
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

            // Block until a packet is available.
            if (osMessageQueueGet(packetQueue_, &sendPacket, nullptr, osWaitForever)
                != osOK) {
                continue;
            }

            const HAL_StatusTypeDef ret = HAL_UART_Transmit_DMA(
                huart_, (uint8_t *)&sendPacket, sendPacket.totalSize());
            if (ret != HAL_OK) {
                // Provide some debug information if DMA transmit couldn't be started
                printf("HAL_UART_Transmit_DMA failed: %d\n\r", static_cast<int>(ret));
                continue;
            }

            // Wait for the DMA completion callback before reusing the stack packet.
            const uint32_t flags
                = osThreadFlagsWait(TX_COMPLETE_FLAG, osFlagsWaitAny, osWaitForever);
            if (flags != TX_COMPLETE_FLAG) {
                printf("UART TX wait failed: 0x%08lx\n\r",
                       static_cast<unsigned long>(flags));
            }
        }
    }


} // namespace


extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == huart_ && taskHandle_ != nullptr) {
        (void)osThreadFlagsSet(taskHandle_, TX_COMPLETE_FLAG);
    }
}


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


    // Thread management
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

        const osStatus_t status = osMessageQueuePut(packetQueue_, &packet, 0, 0);
        if (status != osOK) {
            printf("Failed to queue IMU packet (status=%d)\n\r", status);
        }
    }


    void enqueue_ack()
    {
        assert(isInitialized_);

        DataPacket_raw packet {};
        packet.sync   = SYNC_SEND;
        packet.id     = ePacketID::STM32_ACK;
        packet.length = 0;
        packet.data[packet.length]
            = calculate_crc8((uint8_t *)&packet, packet.totalSize() - 1);

        const osStatus_t status = osMessageQueuePut(packetQueue_, &packet, 0, 0);
        if (status != osOK) {
            printf("Failed to queue ACK packet (status=%d)\n\r", status);
        }
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

        DataPacket_raw packet {};
        packet.sync   = SYNC_SEND;
        packet.id     = ePacketID::STM32_DEBUG;
        packet.length = len;
        std::memcpy(packet.data, msg, packet.length);
        packet.data[packet.length]
            = calculate_crc8((uint8_t *)&packet, packet.totalSize() - 1);

        const osStatus_t status = osMessageQueuePut(packetQueue_, &packet, 0, 0);
        if (status != osOK) {
            printf("Failed to queue debug message (status=%d)\n\r", status);
        }
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