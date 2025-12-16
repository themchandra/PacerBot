/**
 * @file recv.cpp
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Dec-16-2025
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

    // Manage packet parsing
    enum class eParseState : uint8_t {
        SYNC,
        ID,
        LENGTH,
        DATA,
        CHECKSUM,
    };

    constexpr uint32_t FLAGS_VALUE {0x01};
    constexpr uint32_t FLAG_TIMEOUT_MS {100};
    constexpr uint32_t QUEUE_TIMEOUT_MS {100};

    // Receiving buffers
    constexpr int RX_BUF_SIZE {512};
    uint8_t rxBuf_[RX_BUF_SIZE] {};

    // Buffer tracking
    uint16_t curIdx_ {};
    uint16_t newIdx_ {};

    // Track parsing state
    uart::DataPacket_raw dataPacket_ {};
    eParseState curState_ {};
    uint8_t curPos_ {};

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
        // TODO: Error Checking #4
        osMessageQueuePut(packetQueue_, &dataPacket_, 0, 0);
    }

    /**
     * @brief Validate the current byte based on the state
     * @param state The current state of parsing in a packet
     * @param byte The current byte in the data buffer
     * @return true if valid, false otherwise
     */
    bool validateByte(eParseState state, uint8_t byte)
    {
        bool isValid {false};
        uint8_t calcCRC {};

        switch (state) {
        case eParseState::SYNC:
            dataPacket_ = uart::DataPacket_raw {};
            if (byte == uart::SYNC_RECV) {
                isValid = true;
            }
            break;

        case eParseState::ID:
            if (byte >= static_cast<uint8_t>(uart::ePacketID::CMD_MOTOR)
                && byte < static_cast<uint8_t>(uart::ePacketID::TOTAL)) {
                isValid = true;
            }
            break;

        case eParseState::LENGTH:
            if (byte > 0 && byte <= uart::DATA_MAX_SIZE) {
                isValid = true;
            }
            break;

        case eParseState::DATA:
            isValid = true;
            break;

        case eParseState::CHECKSUM:
            calcCRC = uart::calculate_crc8(reinterpret_cast<uint8_t *>(&dataPacket_),
                                           dataPacket_.totalSize() - 1);
            if (byte == calcCRC) {
                isValid = true;
            }
            break;

        default:
            break;
        }

        return isValid;
    }


    /**
     * @brief Update tracking parsing state variables (cursState, curPos)
     */
    void updateState(bool isByteValid)
    {
        if (isByteValid) {
            switch (curState_) {
            case eParseState::SYNC:
            case eParseState::ID:
            case eParseState::LENGTH:
                curState_ = static_cast<eParseState>(static_cast<uint8_t>(curState_) + 1);
                curPos_++;
                break;

            case eParseState::DATA:
                // If the current index position is the byte before checksum
                // sync, id, length not included
                if (curPos_ - 3 == dataPacket_.length - 1) {
                    curState_ = eParseState::CHECKSUM;
                }
                curPos_++;
                break;

            case eParseState::CHECKSUM:
                curState_ = eParseState::SYNC;
                curPos_   = 0;
                break;

            default:
                break;
            }

        } else {
            curState_ = eParseState::SYNC;
            curPos_   = 0;
        }
    }


    void parseBuffer()
    {
        // Circular DMA: 2 situations
        // 1: The DMA is ahead
        // - curIdx <= newIdx, the difference returns the number of bytes we need to parse
        // 2: DMA looped around
        // - curIdx > newIdx, the different returns
        uint16_t newBytes {};
        if (curIdx_ <= newIdx_) {
            newBytes = newIdx_ - curIdx_;

        } else {
            newBytes = RX_BUF_SIZE - curIdx_ + newIdx_;
        }

        // No new bytes & edge case: current index reset to 0 and newIdx_ still at 512
        if (newBytes <= 0 || (newIdx_ - curIdx_) == RX_BUF_SIZE) {
            return;
        }

        // TODO: Peek before commit parser where we wait for stream to contain at least a
        // 		 packet then process it

        // PARSER IMPLEMENTATION
        // Perform a byte by byte buffer parsing
        // - Track which where in the packet via curState
        // - For sync, id, length, & checksum, verify if its valid before copying
        // - If valid, process it through another function, reset
        // - If not valid, reset
        // - For reset, 3 variables: curState, curPos, totalLen
        // - Once each byte is parsed, ensure curIdx is incremented properly, curState &
        // 	 curPos is updated
        uint8_t *dataPacket_ptr = reinterpret_cast<uint8_t *>(&dataPacket_);

        for (uint16_t curByte {}; curByte < newBytes; curByte++) {
            bool isByteValid {validateByte(curState_, rxBuf_[curIdx_])};

            if (isByteValid == true) {
                // Copy data from receiving buffer
                dataPacket_ptr[curPos_] = rxBuf_[curIdx_];

                // Valid and checksum is done, add to freertos queue
                if (curState_ == eParseState::CHECKSUM) {
                    addToQueue();
                }
            }

            // Increment things
            updateState(isByteValid);              // Packet state
            curIdx_ = (curIdx_ + 1) % RX_BUF_SIZE; // Receiving buffer
        }
    }


    void threadLoop(void *argument)
    {
        if (argument) {
        }

        // Either waits for flags or timeout to trigger, then parse.
        while (isTaskRunning_) {
            uint32_t flags
                = osThreadFlagsWait(FLAGS_VALUE, osFlagsWaitAny, FLAG_TIMEOUT_MS);

            // If timed out, update DMA position (via polling)
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
        huart_ = huart;
        // TODO: Error Checking #1
        packetQueue_   = osMessageQueueNew(MAX_QUEUE_SIZE, sizeof(DataPacket_raw), NULL);
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
        // TODO: Error Checking #2
        HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxBuf_, RX_BUF_SIZE);
        isTaskRunning_ = true;
        taskHandle_    = osThreadNew(threadLoop, NULL, &task_att_);
        if (taskHandle_ == nullptr) {
            isTaskRunning_ = false;
            printf("Failed to create recv task (osThreadNew returned NULL)\n\r");
        }
    }


    void stop()
    {
        assert(isInitialized_);
        isTaskRunning_ = false;
        HAL_UART_DMAStop(huart_);
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
        if (osMessageQueueGet(packetQueue_, packet, NULL, timeout_ms) == osOK) {
            return true;
        }
        return false;
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


} // namespace uart::recv