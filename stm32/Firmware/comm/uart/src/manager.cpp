/**
 * @file uart_comm.cpp
 * @brief Manages UART communication between Radxa & STM32
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#include "comm/uart/manager.h"

#include <atomic>
#include <cassert>
#include <memory>
#include <queue>
#include <thread>


namespace {
    bool isInitialized_ {false};
} // namespace


namespace uart::manager {
    void init(UART_HandleTypeDef *huart)
    {
        assert(!isInitialized_);
        send::init(huart);
        recv::init(huart);
        isInitialized_ = true;
    }


    void deinit()
    {
        assert(isInitialized_);
        send::deinit();
        recv::deinit();
        isInitialized_ = false;
    }


    void start()
    {
        assert(isInitialized_);
        recv::start();
        send::start();
    }


    void stop()
    {
        assert(isInitialized_);
        recv::stop();
        send::stop();
    }


    eRunStatus isRunning()
    {
        assert(isInitialized_);
        bool recvStatus {recv::isRunning()};
        bool sendStatus {send::isRunning()};

        eRunStatus status {eRunStatus::RUNNING};
        if (!recvStatus) {
            status = eRunStatus::RECV_STOPPED;
        }

        if (!sendStatus) {
            status = eRunStatus::SEND_STOPPED;
        }

        if (!recvStatus && !sendStatus) {
            status = eRunStatus::BOTH_STOPPED;
        }

        return status;
    }

} // namespace uart::manager