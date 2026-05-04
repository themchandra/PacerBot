/**
 * @file recv.cpp
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date May-03-2026
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
    constexpr uint32_t FLAG_TIMEOUT_MS {10};
    constexpr uint32_t QUEUE_TIMEOUT_MS {0}; // Non-Blocking put

    // Receiving buffers
    constexpr uint16_t RX_BUF_SIZE {1024};
    constexpr uint16_t RX_BUF_MASK {RX_BUF_SIZE - 1};
    uint8_t rxBuf_[RX_BUF_SIZE] {};

    // Buffer tracking
    uint16_t curIdx_ {}; // Updated by parsing thread
    uint16_t dmaIdx_ {}; // Updated by DMA callback

    // Dropped packet counter for observability
    uint32_t droppedPackets_ {};

    // Message queue for parsed packets
    osMessageQueueId_t packetQueue_;

    // Task definition
    std::atomic_bool isTaskRunning_ {false};
    osThreadId_t taskHandle_ {nullptr};
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


    /**
     * @brief Enqueue a validated packet onto the packet queue.
     * @param packet The validated packet to enqueue.
     */
    void addToQueue(const uart::DataPacket_raw &packet)
    {
        // Uses a zero timeout to avoid stalling
        const osStatus_t status
            = osMessageQueuePut(packetQueue_, &packet, 0, QUEUE_TIMEOUT_MS);
        if (status == osOK) {
            return;
        }

        // If full, drop the oldest packet and rety enqueue
        if (status == osErrorResource) {
            uart::DataPacket_raw discarded {};
            const bool isDropped
                = (osMessageQueueGet(packetQueue_, &discarded, NULL, QUEUE_TIMEOUT_MS)
                   == osOK);

            if (isDropped) {
                const osStatus_t retryStatus
                    = osMessageQueuePut(packetQueue_, &packet, 0, QUEUE_TIMEOUT_MS);
                if (retryStatus == osOK) {
                    droppedPackets_++;
                    printf("Queue full, dropped oldest (dropped=%lu)\n\r",
                           droppedPackets_);
                    return;
                }
            }
        }

        droppedPackets_++;
        printf("Failed to queue packet (status=%d, dropped=%lu)\n\r",
               static_cast<int>(status), droppedPackets_);
    }


    /**
     * @brief Calculate CRC8 over a wrapped region in the RX buffer.
     * @param startIdx Buffer index to start from.
     * @param length Number of bytes to checksum.
     * @return CRC8 value for the wrapped region.
     */
    uint8_t calculateWrappedCrc(uint16_t startIdx, uint16_t length)
    {
        const uint16_t bytesUntilWrap = RX_BUF_SIZE - startIdx;
        if (length <= bytesUntilWrap) {
            // No wrap
            return uart::calculate_crc8(&rxBuf_[startIdx], length);
        }

        // Wrap: two contiguous regions
        const uint8_t crc = uart::calculate_crc8(&rxBuf_[startIdx], bytesUntilWrap);
        return uart::calculate_crc8(rxBuf_, length - bytesUntilWrap, crc);
    }


    /**
     * @brief Construct and verify checksum.
     * @details Assumes sync, length, and ID were already checked by the parser.
     * @param packet The packet to fill.
     * @param startIdx The buffer index of the sync byte.
     * @param packetId The packet ID byte.
     * @param payloadLen The payload length.
     * @return true if valid, false otherwise.
     */
    bool verifyAndConstruct(uart::DataPacket_raw &packet, uint16_t startIdx,
                            uint8_t syncByte, uint8_t packetId, uint8_t payloadLen)
    {
        const uint16_t packetLen
            = static_cast<uint16_t>(uart::HEADER_SIZE + payloadLen + uart::CRC_SIZE);
        const uint16_t crcIdx = (startIdx + packetLen - uart::CRC_SIZE) & RX_BUF_MASK;

        // Calculate CRC over the entire packet (excluding the CRC byte itself)
        const uint8_t expectedCRC
            = calculateWrappedCrc(startIdx, packetLen - uart::CRC_SIZE);

        if (rxBuf_[crcIdx] != expectedCRC) {
            return false;
        }

        // ---- Packet construction ----
        // Header
        packet.sync   = syncByte;
        packet.id     = static_cast<uart::ePacketID>(packetId);
        packet.length = payloadLen;
        // Copy payload into packet data array for non ACK packets
        if (payloadLen > 0) {
            const uint16_t dataStartIdx = (startIdx + uart::HEADER_SIZE) & RX_BUF_MASK;
            const uint16_t dataEndIdx   = (dataStartIdx + payloadLen) & RX_BUF_MASK;

            if (dataEndIdx > dataStartIdx) {
                // No wrap-around
                memcpy(packet.data, &rxBuf_[dataStartIdx], payloadLen);
            } else {
                // Wrap-around: copy in two parts
                const uint16_t firstPart = RX_BUF_SIZE - dataStartIdx;
                memcpy(packet.data, &rxBuf_[dataStartIdx], firstPart);
                memcpy(packet.data + firstPart, rxBuf_, payloadLen - firstPart);
            }
        }
        packet.data[payloadLen] = rxBuf_[crcIdx];

        return true;
    }


    /**
     * @brief Check whether a packet candidate is valid.
     * @param startIdx Buffer index of the candidate sync byte.
     * @param dataLen Remaining bytes available from startIdx.
     * @return -1 if invalid, 0 if the packet is incomplete, or the packet
     *         length if the packet is valid.
     */
    int16_t validateData(uint16_t startIdx, uint16_t dataLen)
    {
        const uint8_t syncByte = rxBuf_[startIdx];

        if (syncByte != uart::SYNC_RECV) {
            return -1;
        }

        // Need at least a header (3 bytes: sync, id, len)
        if (uart::HEADER_SIZE > dataLen) {
            return 0;
        }

        const uint8_t packetId = rxBuf_[(startIdx + 1) & RX_BUF_MASK];
        if (packetId < static_cast<uint8_t>(uart::ePacketID::CMD_MOTOR)
            || packetId >= static_cast<uint8_t>(uart::ePacketID::TOTAL)) {
            return -1;
        }

        const uint8_t payloadLen = rxBuf_[(startIdx + 2) & RX_BUF_MASK];
        if (payloadLen >= uart::DATA_MAX_SIZE) {
            return -1;
        }

        // Check for ACK packets
        if (payloadLen == 0
            && packetId != static_cast<uint8_t>(uart::ePacketID::RAD_ACK)) {
            return -1;
        }

        const uint16_t packetLen
            = static_cast<uint16_t>(uart::HEADER_SIZE + payloadLen + uart::CRC_SIZE);
        if (packetLen > dataLen) {
            return 0;
        }

        // A full packet is available, build packet & verify checksum
        uart::DataPacket_raw packet {};
        if (verifyAndConstruct(packet, startIdx, syncByte, packetId, payloadLen)) {
            addToQueue(packet);
            return static_cast<int16_t>(packetLen);
        } else {
            return -1; // Assume data is invalid
        }
    }


    void parseBuffer()
    {
        const uint16_t dmaIdx = dmaIdx_;

        // Calculate available bytes in circular buffer
        uint16_t newBytes {};
        if (curIdx_ <= dmaIdx) {
            newBytes = dmaIdx - curIdx_; // Current index behind DMA
        } else {
            newBytes = RX_BUF_SIZE - curIdx_ + dmaIdx; // DMA index wrapped
        }

        if (newBytes == 0) {
            return;
        }

        // newBytes >= RX_BUF_SIZE -> DMA write is ahead of reader
        // Overflow, reset the read pointer to the current DMA position
        if (newBytes >= RX_BUF_SIZE) {
            curIdx_ = dmaIdx;
            printf("recv buffer overflow\n\r");
            return;
        }

        uint16_t consumedBytes {};
        while (consumedBytes < newBytes) {
            const uint16_t idx = (curIdx_ + consumedBytes) & RX_BUF_MASK;

            const int16_t validBytes = validateData(idx, newBytes - consumedBytes);
            if (validBytes == -1) {
                consumedBytes++; // Skip to the next sync byte

            } else if (validBytes == 0) {
                break; // Stop & wait for next callback

            } else {
                consumedBytes += validBytes; // Advanced indexer to next unread byte
            }
        }

        // Advance the read pointer by the number of bytes consumed.
        // If the loop breaks early from an incomplete packet, the unprocessed
        // bytes be available next iteration.
        curIdx_ = (curIdx_ + consumedBytes) & RX_BUF_MASK;
    }


    void threadLoop(void *argument)
    {
        (void)argument;

        while (isTaskRunning_) {
            // Blocks until updateBufInd() is called
            const uint32_t flags
                = osThreadFlagsWait(FLAGS_VALUE, osFlagsWaitAny, FLAG_TIMEOUT_MS);

            // If timed out, update DMA position via polling
            if (flags != FLAGS_VALUE) {
                dmaIdx_ = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart_->hdmarx);
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
        dmaIdx_ = 0;

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
        dmaIdx_ = index;
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


    uint32_t getDroppedCount()
    {
        assert(isInitialized_);
        return droppedPackets_;
    }

} // namespace uart::recv