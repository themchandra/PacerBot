#include "comm/uart/manager.h"
#include "comm/uart/send.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

int main()
{
    std::cout << "Initializing UART\n";
    uart::manager::init();

    std::cout << "Starting UART manager\n";
    uart::manager::start();

    constexpr std::array<uint8_t, 16> message {
        'H', 'e', 'l', 'l', 'o', ' ', 'f', 'r',
        'o', 'm', ' ', 'L', 'i', 'n', 'u', 'x'
    };

    std::cout << "Enqueuing HOST_DEBUG packet\n";
    uart::send::enqueue(
        uart::DataPacket(uart::ePacketID::HOST_DEBUG, message));

    // Give the background send thread time to process the queue.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Packet queued\n";

    uart::manager::stop();
    uart::manager::deinit();

    return 0;
}
