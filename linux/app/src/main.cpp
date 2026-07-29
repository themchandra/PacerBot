#include "comm/uart/manager.h"

#include <array>
#include <cstdint>
#include <iostream>

/**
 * Minimal UART test matching the STM32 app_main:
 * send a HOST_DEBUG packet and wait for its STM32_ACK.
 */
int main()
{

    uart::manager::init();
    auto ack = uart::recv::subscribe({uart::ePacketID::STM32_ACK});
    uart::manager::start();

    constexpr std::array<uint8_t, 16> message {
        'H', 'e', 'l', 'l', 'o', ' ', 'f', 'r', 'o', 'm', ' ', 'L', 'i', 'n', 'u', 'x'
    };
    uart::send::enqueue(
        uart::DataPacket(uart::ePacketID::HOST_DEBUG, message));

    ack->pop();
    std::cout << "Received ACK from STM32\n";

    uart::manager::stop();
    uart::manager::deinit();

    return 0;
}
