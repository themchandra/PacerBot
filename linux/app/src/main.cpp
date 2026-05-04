#include "hal/encoders.h"
#include "hal/motors.h"
#include "state_machine.h"
#include <chrono>
#include <iostream>
#include <string>
#include <termios.h>
#include <thread>

#include "comm/uart/manager.h"

#include "timing.h"

void test()
{
    std::cout << "Test thread!\n";

    int counter {};
    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        // SENDING
        std::string str = "Hello world " + std::to_string(counter++);

        auto packet = uart::DataPacket(
            uart::ePacketID::HOST_ACK,
            std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(str.data()),
                                     str.size()));
        uart::send::enqueue(std::move(packet));
        timing::sleepForMs(1000);
    }
}

/**
 * Main demonstration program for PacerBot state machine
 * This program runs a simple sequence to test the different states
 */
int main()
{
    uart::manager::init();
    uart::manager::start();

    timing::init();

    std::thread thread_(test);

    std::cout << "Init done!\n";

    // Subscribe to all incoming packet types
    auto subscriber = uart::recv::subscribe(
        {uart::ePacketID::TELEM_IMU, uart::ePacketID::TELEM_ULT,
         uart::ePacketID::TELEM_ENC, uart::ePacketID::TELEM_PID,
         uart::ePacketID::TELEM_BATTERY, uart::ePacketID::STM32_STATUS,
         uart::ePacketID::STM32_ACK, uart::ePacketID::STM32_DEBUG});

    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        // RECEIVING - pop() blocks until a packet is available
        try {
            auto packet = subscriber->pop();

            std::cout << "\nData received!! Printing packet...\n";
            const std::vector<uint8_t> &data = packet.getData();

            std::cout << "Packet data: ";
            for (uint8_t byte : data) {
                std::cout << static_cast<char>(byte);
            }
            std::cout << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Error receiving packet: " << e.what() << std::endl;
        }
    }

    uart::recv::unsubscribe(subscriber);
    timing::deinit();
    uart::manager::deinit();
}