/**
 * @file recv.cpp
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-06-2025
 */

#include "cmsis_os.h"
#include "comm/uart/callbacks.h"
#include "comm/uart/packet_info.h"
#include "comm/uart/recv.h"

#include <atomic>
#include <cassert>
#include <string.h>

namespace {
    bool isInitialized_ {false};
    UART_HandleTypeDef *huart_;

    // Receiving buffers
    constexpr int RX_BUF_SIZE {512};
    uint8_t rxBuf[RX_BUF_SIZE] {};

    // Buffer tracking
    uint16_t curIdx {};
    uint16_t newIdx {};

    // Threading
    std::atomic_bool isTaskRunning_ {false};
    osSemaphoreId_t semTaskLoop_;

    // Task definition
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

    // Manage packet parsing
    enum class eParseState : uint8_t {
        SYNC,
        ID,
        LENGTH,
        DATA,
        CHECKSUM,
    };

    // Track parsing state
    uart::DataPacket_raw dataPacket {};
    eParseState curState {};
    uint8_t curPos {};

    uart::DataPacket_raw sendPacket {};

    void transmitPacket(eParseState state)
    {
        const char *msg;
        switch (state) {
        case eParseState::SYNC:
            msg = "SYNC!";
            break;
        case eParseState::ID:
            msg = "ID!";
            break;
        case eParseState::DATA:
            msg = "STM32 - Data received, added to queue!";
            break;
        case eParseState::LENGTH:
            msg = "LENGTH!";
            break;
        case eParseState::CHECKSUM:
            msg = "CHECKSUM!";
            break;
        default:
            msg = "";
            break;
        }
        sendPacket.sync   = uart::SYNC_SEND;
        sendPacket.id     = uart::ePacketID::TELEM_IMU;
        sendPacket.length = strlen(msg);
        memcpy(sendPacket.data, msg, sendPacket.length);

        sendPacket.data[sendPacket.length]
            = uart::calculate_crc8((uint8_t *)&sendPacket, sendPacket.totalSize() - 1);

        HAL_UART_Transmit_DMA(huart_, (uint8_t *)&sendPacket, sendPacket.totalSize());
    }

    /**
     * @brief Validate the current byte based on the state
     * @param state The current state of parsing in a packet
     * @param byte The current byte in the data buffer
     * @return true if valid, false otherwise
     */
    bool validateByteState(eParseState state, uint8_t byte)
    {
        bool isValid {false};
        uint8_t calcCRC {};

        switch (state) {
        case eParseState::SYNC:
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
            calcCRC = uart::calculate_crc8(reinterpret_cast<uint8_t *>(&dataPacket),
                                           dataPacket.totalSize() - 1);
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
            switch (curState) {
            case eParseState::SYNC:
            case eParseState::ID:
            case eParseState::LENGTH:
                curState = static_cast<eParseState>((static_cast<uint8_t>(curState) + 1));
                curPos++;
                break;

            case eParseState::DATA:
                if (curPos - 3 == dataPacket.length - 1) {
                    curState = eParseState::CHECKSUM;
                }
                curPos++;
                break;

            case eParseState::CHECKSUM:
                curState = eParseState::SYNC;
                curPos   = 0;

            default:
                break;
            }

        } else {
            curState = eParseState::SYNC;
            curPos   = 0;
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
        if (curIdx <= newIdx) {
            newBytes = newIdx - curIdx;

        } else {
            newBytes = RX_BUF_SIZE - curIdx + newIdx;
        }

        if (newBytes <= 0) {
            return;
        }

        // IMPLEMENTATION
        // Perform a byte by byte buffer parsing
        // - Track which where in the packet via curState
        // - For sync, id, length, & checksum, verify if its valid before copying
        // - If valid, process it through another function, reset
        // - If not valid, reset
        // - For reset, 3 variables: curState, curPos, totalLen
        // - Once each byte is parsed, ensure curIdx is incremented properly, curState &
        // 	 curPos is updated
        for (uint16_t curByte {}; curByte < newBytes; curByte++) {
            bool isByteValid {validateByteState(curState, rxBuf[curIdx])};

            if (isByteValid == true) {
                // Copy data from receiving buffer
                uint8_t *dataPacket_ptr = reinterpret_cast<uint8_t *>(&dataPacket);
                dataPacket_ptr[curPos]  = rxBuf[curIdx];

                // Valid and checksum is done, add to freertos queue
                if (curState == eParseState::CHECKSUM) {
                    transmitPacket(eParseState::DATA);
                }
            }

            // Increment things
            updateState(isByteValid);            // Packet state
            curIdx = (curIdx + 1) % RX_BUF_SIZE; // Receiving buffer
        }
    }


    void threadLoop(void *argument)
    {
        if (argument) {
        }

        constexpr int FLAG_TIMEOUT_MS {100};

        // Either waits for flags or timeout to trigger, then parse.
        while (isTaskRunning_) {
            uint32_t flags
                = osThreadFlagsWait(static_cast<uint32_t>(uart::recv::eFlags::CALLBACK),
                                    osFlagsWaitAny, FLAG_TIMEOUT_MS);

            // If timed out, update DMA position (via polling)
            if (flags != static_cast<uint32_t>(uart::recv::eFlags::CALLBACK)) {

                // Calculates how many bytes have been received in the DMA buffer
                newIdx = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart_->hdmarx);
            }

            parseBuffer();
        }

        // Simulates a "thread join" for confirming that the loop ends
        osSemaphoreRelease(semTaskLoop_);
    }

} // namespace


namespace uart::recv {
    void init(UART_HandleTypeDef *huart)
    {
        assert(!isInitialized_);
        huart_ = huart;
        callbacks::set_huart(callbacks::eUARTPort::UART_2, huart);
        semTaskLoop_   = osSemaphoreNew(1, 0, NULL);
        isInitialized_ = true;
    }


    void deinit()
    {
        assert(isInitialized_);
        isInitialized_ = false;
    }


    void start()
    {
        assert(isInitialized_);
        HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxBuf, RX_BUF_SIZE);
        taskHandle_    = osThreadNew(threadLoop, NULL, &task_att_);
        isTaskRunning_ = true;
    }


    void stop()
    {
        assert(isInitialized_);
        isTaskRunning_ = false;

        // Wait until loop ends then "join" with other thread
        osSemaphoreAcquire(semTaskLoop_, osWaitForever);
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
        newIdx = index;
        osThreadFlagsSet(taskHandle_, static_cast<uint32_t>(eFlags::CALLBACK));
    }

} // namespace uart::recv