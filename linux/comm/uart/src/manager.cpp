/**
 * @file uart_comm.cpp
 * @brief Manages UART communication between Radxa & STM32
 * @author Hayden Mai
 * @date Oct-17-2025
 */

#include "comm/uart/config.h"
#include "comm/uart/manager.h"
#include "comm/uart/recv.h"
#include "comm/uart/send.h"

#include "hal/SerialUART.h"

#include <atomic>
#include <cassert>
#include <memory>
#include <queue>
#include <thread>


namespace {
    bool isInitialized_ {false};

    // Shared pointer for recv and send modules to access
    auto uartPtr_ = std::make_shared<SerialUART>(
        uart::config::UART_DEVICE, uart::config::BAUDRATE, uart::config::TIMEOUT_SEC);

} // namespace


namespace uart::manager {
    void init()
    {
        assert(!isInitialized_);

        // Initialize & open UART
        try {
            assert(uartPtr_ != nullptr);
            uartPtr_->openPort();

            send::init(uartPtr_);
            recv::init(uartPtr_);
            isInitialized_ = true;

        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }


    void deinit()
    {
        assert(isInitialized_);

        try {
            uartPtr_->closePort();

            send::deinit();
            recv::deinit();
            isInitialized_ = false;

        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
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