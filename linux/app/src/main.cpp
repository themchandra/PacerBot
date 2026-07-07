#include "comm/uart/packet_info.h"
#include "hal/encoders.h"
#include "hal/motors.h"
#include "manual_control.h"
#include "state_machine.h"
#include <chrono>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <sys/types.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

#ifdef PACERBOT_HAS_UART
#include "comm/uart/manager.h"
#endif

#include "timing.h"


/**
 * Test for manual control 
 */
int main()
{
#ifdef PACERBOT_HAS_UART
    uart::manager::init();
    uart::manager::start();

    timing::init();

    ManualControl control;
    std::cout << "Test thread!\n";

    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        updateManualControl(control);
        uint8_t command = encodeManualControl(control);

        // SENDING
        auto packet = uart::DataPacket(uart::ePacketID::CMD_MCTL,
                                       std::span<const uint8_t>(&command, 1));
        uart::send::enqueue(std::move(packet));
        timing::sleepForMs(100);
    }
    timing::deinit();
    uart::manager::deinit();
#endif
    return 0;

}
