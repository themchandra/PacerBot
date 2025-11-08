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
    int counter {};
    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        // SENDING
        std::string str = "Hello world " + std::to_string(counter++);

        auto packet = uart::DataPacket(
            uart::ePacketID::RAD_ACK,
            std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(str.data()),
                                     str.size()));
        uart::send::enqueue(packet);
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
    uart::EventFlag &eventFlag {uart::recv::getEventFlag()};

    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        // RECEIVING
        eventFlag.wait();
        while (uart::recv::isQueueEmpty() == false) {
            auto newPacket = uart::recv::dequeue();

            if (newPacket.has_value()) {
                std::cout << "\nData received!! Printing packet...\n";
                auto packet = newPacket.value();

                // TODO: Print from packet class
                const std::vector<uint8_t> &data = packet.getData();

                std::cout << "Packet data: ";
                for (uint8_t byte : data) {
                    std::cout << static_cast<char>(byte);
                }
                std::cout << std::endl;
            }
        }
    }

    timing::deinit();
    uart::manager::deinit();
}