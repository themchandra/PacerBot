/**
 * @file recv.cpp
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date May-01-2026
 */

#include "comm/uart/recv.h"

#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>

namespace {
    bool isInitialized_ {false};
    UART_HandleTypeDef *huart_;

    constexpr int32_t FLAGS_VALUE {0x01};
    constexpr uint32_t FLAG_TIMEOUT_MS {100};
    constexpr uint32_t QUEUE_TIMEOUT_MS {100};

    // Receiving buffers
    constexpr uint16_t RX_BUF_SIZE {1024};
    constexpr uint16_t RX_BUF_MASK {
        RX_BUF_SIZE - 1}; // Bitmask for circular buffer (requires power-of-2 size)
    uint8_t rxBuf_[RX_BUF_SIZE] {};

    // Buffer tracking - volatile for shared access between ISR and thread
    volatile uint16_t curIdx_ {}; // Updated by parsing thread
    volatile uint16_t newIdx_ {}; // Updated by DMA callback

    // Parsed packet storage
    uart::DataPacket_raw dataPacket_ {};

    // Message queue for parsed packets
    osMessageQueueId_t packetQueue_;

    // Task definition
    std::atomic_bool isTaskRunning_ {false};
    osThreadId_t taskHandle_;
    constexpr uint32_t STACK_SIZE_BYTES {512};
    constexpr osThreadAttr_t task_att_ {
        .name       = "recvTask",
        .attr_bits  = 0,
        .cb_mem     = nullptr,
        .cb_size    = 0,
        .stack_mem  = nullptr,
        .stack_size = STACK_SIZE_BYTES,
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0,
    };


    void addToQueue()
    {
        const osStatus_t status
            = osMessageQueuePut(packetQueue_, &dataPacket_, 0, QUEUE_TIMEOUT_MS);
        if (status != osOK) {
            printf("Failed to queue packet (status=%d)\n\r", status);
        }
    }

    /**
     * @brief Validate a candidate packet using the same rules as the Linux parser.
     * @param packet The candidate packet.
     * @return true if valid, false otherwise.
     */
    bool validatePacket(const uart::DataPacket_raw &packet)
    {
        if (packet.sync != uart::SYNC_RECV) {
            return false;
        }

        const auto packetId = static_cast<uint8_t>(packet.id);
        if (packetId < static_cast<uint8_t>(uart::ePacketID::CMD_MOTOR)
            || packetId >= static_cast<uint8_t>(uart::ePacketID::TOTAL)) {
            return false;
        }

        if (packet.length == 0 || packet.length >= uart::DATA_MAX_SIZE) {
            return false;
        }

        const auto *rawBytes = reinterpret_cast<const uint8_t *>(&packet);
        const uint8_t expectedCRC
            = uart::calculate_crc8(rawBytes, packet.totalSize() - 1);
        return packet.data[packet.length] == expectedCRC;
    }


    void parseBuffer()
    {
        // Cache volatile indices to avoid repeated volatile reads
        const uint16_t curIdxCache = curIdx_;
        const uint16_t newIdxCache = newIdx_;

        // Calculate available bytes in circular buffer
        uint16_t newBytes {};
        if (curIdxCache <= newIdxCache) {
            newBytes = newIdxCache - curIdxCache;
        } else {
            newBytes = RX_BUF_SIZE - curIdxCache + newIdxCache;
        }

        // No new bytes or buffer completely full
        if (newBytes == 0 || newBytes == RX_BUF_SIZE) {
            return;
        }

        uint16_t consumedBytes {};
        constexpr uint16_t HEADER_SIZE {3};

        while (consumedBytes < newBytes) {
            const uint16_t idx     = (curIdxCache + consumedBytes) & RX_BUF_MASK;
            const uint8_t syncByte = rxBuf_[idx];
            if (syncByte != uart::SYNC_RECV) {
                consumedBytes++;
                continue;
            }

            // Check if we have full header
            if (consumedBytes + HEADER_SIZE > newBytes) {
                break;
            }

            const uint8_t payloadLen
                = rxBuf_[(curIdxCache + consumedBytes + 2) & RX_BUF_MASK];
            // Fail-fast: validate length before attempting to construct packet
            if (payloadLen == 0 || payloadLen >= uart::DATA_MAX_SIZE) {
                consumedBytes++;
                continue;
            }

            const uint16_t packetLen
                = static_cast<uint16_t>(HEADER_SIZE + payloadLen + 1);
            // Check if complete packet is available
            if (consumedBytes + packetLen > newBytes) {
                break;
            }

            // Construct packet efficiently
            uart::DataPacket_raw packet {};
            packet.sync = syncByte;
            packet.id   = static_cast<uart::ePacketID>(
                rxBuf_[(curIdxCache + consumedBytes + 1) & RX_BUF_MASK]);
            packet.length = payloadLen;

            // Optimized payload copy: use memcpy when buffer doesn't wrap
            const uint16_t dataStartIdx
                = (curIdxCache + consumedBytes + HEADER_SIZE) & RX_BUF_MASK;
            const uint16_t dataEndIdx = (dataStartIdx + payloadLen) & RX_BUF_MASK;

            if (dataEndIdx > dataStartIdx) {
                // No wrap-around: use fast memcpy
                memcpy(packet.data, &rxBuf_[dataStartIdx], payloadLen);
            } else {
                // Wrap-around: copy in two parts
                const uint16_t firstPart = RX_BUF_SIZE - dataStartIdx;
                memcpy(packet.data, &rxBuf_[dataStartIdx], firstPart);
                memcpy(packet.data + firstPart, rxBuf_, payloadLen - firstPart);
            }
            // Copy CRC byte
            packet.data[payloadLen]
                = rxBuf_[(curIdxCache + consumedBytes + packetLen - 1) & RX_BUF_MASK];

            if (validatePacket(packet)) {
                dataPacket_ = packet;
                addToQueue();
                consumedBytes += packetLen;
                continue;
            }

            consumedBytes++;
        }

        // Use bitwise AND instead of modulo
        curIdx_ = (curIdxCache + consumedBytes) & RX_BUF_MASK;
    }


    void threadLoop(void *argument)
    {
        (void)argument; // Suppress unused parameter warning

        // Either waits for flags or timeout to trigger, then parse.
        while (isTaskRunning_) {
            const uint32_t flags
                = osThreadFlagsWait(FLAGS_VALUE, osFlagsWaitAny, FLAG_TIMEOUT_MS);

            // If timed out (no flags set), update DMA position via polling
            if (flags != FLAGS_VALUE) {
                // Calculates how many bytes have been received in the DMA buffer
                newIdx_ = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart_->hdmarx);
            }

            parseBuffer();
        }
    }

} // namespace


namespace uart::recv {
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
        curIdx_ = 0;
        newIdx_ = 0;

        const HAL_StatusTypeDef status
            = HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxBuf_, RX_BUF_SIZE);
        if (status != HAL_OK) {
            printf("Failed to start DMA receive (status=%d)\n\r", status);
            return;
        }

        isTaskRunning_ = true;
        taskHandle_    = osThreadNew(threadLoop, NULL, &task_att_);
        if (taskHandle_ == nullptr) {
            isTaskRunning_ = false;
            HAL_UART_DMAStop(huart_);
            printf("Failed to create recv task (osThreadNew returned NULL)\n\r");
        }
    }


    void stop()
    {
        assert(isInitialized_);
        isTaskRunning_ = false;
        HAL_UART_DMAStop(huart_);
        osThreadTerminate(taskHandle_);
        taskHandle_ = nullptr;
    }


    bool isRunning()
    {
        assert(isInitialized_);
        return isTaskRunning_;
    }


    void updateBufInd(uint16_t index)
    {
        assert(isInitialized_);
        newIdx_ = index;
        osThreadFlagsSet(taskHandle_, FLAGS_VALUE);
    }


    bool dequeue(DataPacket_raw *packet, uint32_t timeout_ms)
    {
        assert(isInitialized_);
        return osMessageQueueGet(packetQueue_, packet, NULL, timeout_ms) == osOK;
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


} // namespace uart::recv