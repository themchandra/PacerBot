#include "hal/encoders.h"
#include "hal/motors.h"
#include "state_machine.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include "comm/uart/manager.h"
#include "comm/uart/send.h"
#include "comm/ipc/lane_pipe_reader.h"
#include <span>
#include "timing.h"

void test()
{
    std::cout << "Test thread!\n";

    int counter {};
    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        // SENDING
        std::string str = "Hello world " + std::to_string(counter++);

        auto packet = uart::DataPacket(
            uart::ePacketID::HOST_DEBUG,
            std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(str.data()),
                                     str.size()));
        uart::send::enqueue(std::move(packet));
        timing::sleepForMs(100);

        auto ack_packet = uart::DataPacket(uart::ePacketID::HOST_ACK,
                                           std::span<const uint8_t>());
        uart::send::enqueue(std::move(ack_packet));
        timing::sleepForMs(100);
        
    }
}

void sendLaneInput() {
    if (!initializeLanePipe()) {
        std::cerr << "Failed to initialize lane IPC pipe\n";
        return;
    }

    LaneInput input {};

    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        if (readLaneInput(input) && input.valid) {
            float value = input.steering_error;
            uint8_t payload[sizeof(value)];
            std::memcpy(payload, &value, sizeof(value));

            auto packet = uart::DataPacket(
                uart::ePacketID::TELEM_LINE_POS,
                std::span<const uint8_t>(payload, sizeof(payload)));
                

            uart::send::enqueue(std::move(packet));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
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
    std::thread ipc_thread(sendLaneInput);

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

    if (ipc_thread.joinable()) {
        ipc_thread.join();
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}