#include "hal/encoders.h"
#include "hal/motors.h"
#include "state_machine.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include "comm/uart/manager.h"
#include "comm/uart/send.h"
#include "comm/ipc/lane_pipe_reader.h"
#include <span>
#include "timing.h"

namespace {
    constexpr auto kLaneUpdatePeriod = std::chrono::milliseconds(20);
    constexpr size_t kFakeLaneSampleCount {20};

    constexpr size_t kPacketDumpSize {uart::DATA_MAX_SIZE};

    bool hasFlag(int argc, char **argv, std::string_view flag)
    {
        for (int i = 1; i < argc; ++i) {
            if (argv[i] == flag) {
                return true;
            }
        }
        return false;
    }

    void handleLanePacket(float steeringError, bool noUart)
    {
        std::cout << "Fake lane steering_error=" << steeringError << '\n';

        auto packet = uart::DataPacket(
            uart::ePacketID::TELEM_LINE_POS,
            std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(&steeringError),
                                     sizeof(steeringError)));

        if (noUart) {
            uint8_t serialized[kPacketDumpSize] {};
            const size_t serializedSize = packet.serialize(serialized, sizeof(serialized));

            std::cout << "Serialized bytes:";
            for (size_t i = 0; i < serializedSize; ++i) {
                std::cout << ' ' << std::hex << static_cast<int>(serialized[i]);
            }
            std::cout << std::dec << '\n';
            return;
        }

        uart::send::enqueue(std::move(packet));
    }
} // namespace

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

void sendLaneInput(bool useFakeLaneInput, bool noUart)
{
    if (useFakeLaneInput) {
        float steeringError = -0.5f;
        float step          = 0.05f;

        while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
            handleLanePacket(steeringError, noUart);

            steeringError += step;
            if (steeringError >= 0.5f || steeringError <= -0.5f) {
                step = -step;
            }

            std::this_thread::sleep_for(kLaneUpdatePeriod);
        }

        return;
    }

    if (!initializeLanePipe()) {
        std::cerr << "Failed to initialize lane IPC pipe\n";
        return;
    }

    LaneInput input {};

    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        if (readLaneInput(input) && input.valid) {
            handleLanePacket(input.steering_error, noUart);
        }

        std::this_thread::sleep_for(kLaneUpdatePeriod);
    }
}

/**
 * Main demonstration program for PacerBot state machine
 * This program runs a simple sequence to test the different states
 */
int main(int argc, char **argv)
{
    const bool useFakeLaneInput = hasFlag(argc, argv, "--fake-lane");
    const bool noUart           = hasFlag(argc, argv, "--no-uart");

    if (noUart) {
        if (!useFakeLaneInput) {
            std::cerr << "--no-uart requires --fake-lane\n";
            return 1;
        }

        std::cout << "Init done!\n";

        float steeringError = -0.5f;
        float step          = 0.05f;

        for (size_t sampleIndex = 0; sampleIndex < kFakeLaneSampleCount; ++sampleIndex) {
            handleLanePacket(steeringError, true);

            steeringError += step;
            if (steeringError >= 0.5f || steeringError <= -0.5f) {
                step = -step;
            }

            std::this_thread::sleep_for(kLaneUpdatePeriod);
        }

        return 0;
    }

    std::cout << "Init done!\n";

    uart::manager::init();
    uart::manager::start();

    timing::init();

    std::thread thread_(test);
    std::thread ipc_thread(sendLaneInput, useFakeLaneInput, false);

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