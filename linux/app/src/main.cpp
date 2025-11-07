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

/**
 * Main demonstration program for PacerBot state machine
 * This program runs a simple sequence to test the different states
 */
int main()
{
    /*
using namespace std::chrono;
constexpr float dt = 0.01f; // 10 ms = 100 Hz update rate
auto next_tick = steady_clock::now();

// Initialize in IDLE mode
app::set_target_speed(0.0f);

// Run demo sequence for 2 seconds total (200 iterations at 100Hz)
for (int i = 0; i < 200; ++i) {
    // State transitions at specific times
    if (i == 30)  app::set_target_speed(0.5f); // Switch to RUN mode at t=0.30s
    if (i == 100) {
    // Simulate emergency stop at t=1.00s
        app::emergency_stop();
        std::cout << "EMERGENCY STOP ACTIVATED\n";
    }
    if (i == 150) app::set_target_speed(0.0f); // Switch back to IDLE at t=1.50s

    if (i == 170) {
        // Reset from E_STOP at t=1.70s
         app::reset();
         std::cout << "EMERGENCY STOP RESET\n";
    }

    // Update the state machine
    app::tick(dt);

    // Output current iteration and state machine mode
    // Mode values: 0=IDLE, 1=RUN, 2=E_STOP
    // Enhanced output to show simulation in action
    std::cout << "i=" << i
            << " mode=" << static_cast<int>(app::mode())
            << " duty=" << hal::motors::current_left_duty
            << " speed=" << hal::encoders::get_speed()
            << " m/s\n";


    // Sleep until next update time to maintain consistent timing
    next_tick += duration_cast<steady_clock::duration>(duration<float>(dt));
    std::this_thread::sleep_until(next_tick);
}
return 0;
    */

    uart::manager::init();
    uart::manager::start();

    timing::init();

    std::cout << "Init done!\n";
    int counter {};

    while (uart::manager::isRunning() == uart::manager::eRunStatus::RUNNING) {
        // RECEIVING
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
       
        // SENDING
        std::string str = "Hello world " + std::to_string(counter++);

        auto packet = uart::DataPacket(
            uart::ePacketID::RAD_ACK,
            std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(str.data()),
                                     str.size()));
        uart::send::enqueue(packet);

        timing::sleepForMs(1000);
    }

    timing::deinit();
    uart::manager::deinit();
}