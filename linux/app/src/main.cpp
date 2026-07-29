#include "comm/uart/manager.h"

#include <iostream>

int main()
{
    std::cout << "Initializing UART\n";
    uart::manager::init();

    auto sub = uart::recv::subscribe({
        uart::ePacketID::STM32_ACK
    });

    std::cout << "Starting UART manager\n";
    uart::manager::start();

    std::cout << "Waiting for STM32_ACK...\n";

    auto packet = sub->pop();

    std::cout << "Received STM32_ACK\n";

    uart::manager::stop();
    uart::manager::deinit();

    return 0;
}
