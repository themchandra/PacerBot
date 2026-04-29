/**
 * @file send.cpp
 * @brief Handles data transmission via UART DMA
 * @author Hayden Mai
 * @date Dec-16-2025
 */

#include "comm/uart/send.h"

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#include <cstdio>
#include <atomic>
#include <cassert>
#include <cstring>

namespace {
    bool isInitialized_ {false};
    UART_HandleTypeDef *huart_;

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

            // Blocks & waits for packets in queue
            osMessageQueueGet(packetQueue_, &sendPacket, 0, osWaitForever);
            HAL_StatusTypeDef ret = HAL_UART_Transmit_DMA(huart_, (uint8_t *)&sendPacket,
                                                          sendPacket.totalSize());
            if (ret != HAL_OK) {
                // Provide some debug information if DMA transmit couldn't be started
                printf("HAL_UART_Transmit_DMA failed: %d\n\r", static_cast<int>(ret));
            }

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
    }


    void enqueue_debug(const char *msg)
    {
        assert(isInitialized_);

        if (msg == nullptr) {
            return;
        }

        // Cut off string if too long
        constexpr uint8_t MAX_MSG_SIZE {DATA_MAX_SIZE - 1};
        uint8_t len = strlen(msg);
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

        osMessageQueuePut(packetQueue_, &packet, 0, 0);
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