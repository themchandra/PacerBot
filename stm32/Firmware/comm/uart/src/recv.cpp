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
    constexpr uint32_t QUEUE_TIMEOUT_MS {0}; // Non-blocking put

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
            const bool isDropped = uart::recv::dequeue(&discarded, QUEUE_TIMEOUT_MS);

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
     * @brief Validate a candidate packet
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

        // TODO: Check if this is valid for ACK ids as that use 0 length payloads
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
        // Calculate available bytes in circular buffer
        uint16_t newBytes {};
        if (curIdx_ <= dmaIdx_) {
            newBytes = dmaIdx_ - curIdx_; // Current index behind DMA
        } else {
            newBytes = RX_BUF_SIZE - curIdx_ + dmaIdx_; // DMA index wrapped
        }

        if (newBytes == 0) {
            return;
        }

        // newBytes >= RX_BUF_SIZE -> DMA write is ahead of reader
        // Overflow, reset the read pointer to the current DMA position
        if (newBytes >= RX_BUF_SIZE) {
            curIdx_ = dmaIdx_;
            printf("recv buffer overflow\n\r");
            return;
        }

        uint16_t consumedBytes {};
        constexpr uint16_t HEADER_SIZE {3};

        while (consumedBytes < newBytes) {
            // ---- Check if a full packet is available ----
            const uint16_t idx     = (curIdx_ + consumedBytes) & RX_BUF_MASK;
            const uint8_t syncByte = rxBuf_[idx];

            // Loops until a sync byte is detected
            if (syncByte != uart::SYNC_RECV) {
                consumedBytes++;
                continue;
            }

            if (consumedBytes + HEADER_SIZE > newBytes) {
                break; // Stop parsing entirely
            }

            // Have header, get length of potential packet data
            const uint8_t payloadLen
                = rxBuf_[(curIdx_ + consumedBytes + 2) & RX_BUF_MASK];

            // TODO: Check if this is valid for ACK ids as that use 0 length payloads
            if (payloadLen == 0 || payloadLen >= uart::DATA_MAX_SIZE) {
                consumedBytes++;
                continue; // Invalid, go back to finding the next sync byte
            }

            const uint16_t packetLen
                = static_cast<uint16_t>(HEADER_SIZE + payloadLen + 1);

            if (consumedBytes + packetLen > newBytes) {
                break; // Packet not fully available, stop parsing
            }

            // ---- A full packet is available, attempt validation ----
            // Construct packet
            uart::DataPacket_raw packet {};
            packet.sync = syncByte;
            packet.id   = static_cast<uart::ePacketID>(
                rxBuf_[(curIdx_ + consumedBytes + 1) & RX_BUF_MASK]);
            packet.length = payloadLen;

            // Copy payload: use single memcpy when data doesn't wrap, otherwise
            // split into two copies across the buffer boundary
            const uint16_t dataStartIdx
                = (curIdx_ + consumedBytes + HEADER_SIZE) & RX_BUF_MASK;
            const uint16_t dataEndIdx = (dataStartIdx + payloadLen) & RX_BUF_MASK;

            if (dataEndIdx > dataStartIdx) {
                // No wrap-around
                memcpy(packet.data, &rxBuf_[dataStartIdx], payloadLen);
            } else {
                // Wrap-around: copy in two parts
                const uint16_t firstPart = RX_BUF_SIZE - dataStartIdx;
                memcpy(packet.data, &rxBuf_[dataStartIdx], firstPart);
                memcpy(packet.data + firstPart, rxBuf_, payloadLen - firstPart);
            }

            // Copy CRC byte (last byte of the packet)
            packet.data[payloadLen]
                = rxBuf_[(curIdx_ + consumedBytes + packetLen - 1) & RX_BUF_MASK];

            if (validatePacket(packet)) {
                addToQueue(packet);
                consumedBytes += packetLen;
                continue;
            }

            // Packet failed validation — advance by one byte and retry from
            // the next position. curIdx_ will be updated
            consumedBytes++;
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