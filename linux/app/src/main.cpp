#include "comm/uart/manager.h"
#include "comm/uart/packet_info.h"
#include "comm/uart/send.h"
#include "manual_control.h"
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    uart::manager::init();
    uart::manager::start();
    ManualControl control;

    while (true) {
        updateManualControl(control);
        const auto encoded = encodeManualControl(control);
        const std::array<uint8_t, 1> payload { encoded };
        uart::send::enqueue(uart::DataPacket(uart::ePacketID::CMD_MCTL,payload));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    uart::manager::stop();
    uart::manager::deinit();

    return 0;
}