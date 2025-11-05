/**
 * @file recv.cpp
 * @brief Handles incoming packets from UART
 * @author Hayden Mai
 * @date Nov-05-2025
 */

#include "cmsis_os.h"
#include "comm/uart/callbacks.h"
#include "comm/uart/recv.h"

#include <atomic>
#include <cassert>

namespace {
    bool isInitialized_ {false};
    UART_HandleTypeDef *huart_;

    // Receiving buffers
    constexpr int RX_BUF_SIZE {20};
    uint8_t rxBuf[RX_BUF_SIZE] {};

    // Buffer tracking
    uint16_t oldIdx {};
    uint16_t curIdx {};

    constexpr int FLAG_TIMEOUT_MS {100};

    // Threading
    std::atomic_bool isTaskRunning_ {false};
    osSemaphoreId_t semTaskLoop_;

    // Task definition
    osThreadId_t taskHandle_;
    constexpr osThreadAttr_t task_att_ {
        .name       = "recvTask",
        .attr_bits  = 0,
        .cb_mem     = nullptr,
        .cb_size    = 0,
        .stack_mem  = nullptr,
        .stack_size = 128 * 4,
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0,
    };


    void parseBuffer()
    {
        // TODO: parse buffer
    }


    void threadLoop(void *argument)
    {
        if (argument) {
        }

        while (isTaskRunning_) {
            uint32_t flags
                = osThreadFlagsWait(static_cast<uint32_t>(uart::recv::eFlags::CALLBACK),
                                    osFlagsWaitAny, FLAG_TIMEOUT_MS);

            // If timed out, update DMA position (via polling)
            if (flags != static_cast<uint32_t>(uart::recv::eFlags::CALLBACK)) {
				oldIdx = curIdx;

				// Calculates how many bytes have been received in the DMA buffer
				curIdx = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart_->hdmarx);
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
    }


    bool isRunning()
    {
        assert(isInitialized_);
        return isTaskRunning_;
    }


    void updateBufInd(uint16_t index)
    {
        assert(isInitialized_);

        // Save previous index value
        oldIdx = curIdx;
        curIdx = index;

        osThreadFlagsSet(taskHandle_, static_cast<uint32_t>(eFlags::CALLBACK));
    }

} // namespace uart::recv