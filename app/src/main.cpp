#include "state_machine.h"
#include <chrono>
#include <thread>
#include <iostream>

/**
 * Main demonstration program for PacerBot state machine
 * This program runs a simple sequence to test the different states
 */
int main() {
    using namespace std::chrono;
    constexpr float dt = 0.01f; // 10 ms = 100 Hz update rate
    auto next_tick = steady_clock::now();

    // Initialize in IDLE mode
    app::set_target_speed(0.0f);

    // Run demo sequence for 2 seconds total (200 iterations at 100Hz)
    for (int i = 0; i < 200; ++i) {
        // State transitions at specific times
        if (i == 30)  app::set_target_speed(0.5f); // Switch to RUN mode at t=0.30s
        if (i == 150) app::set_target_speed(0.0f); // Switch back to IDLE at t=1.50s

        // Update the state machine
        app::tick(dt);

        // Output current iteration and state machine mode
        // Mode values: 0=IDLE, 1=RUN, 2=E_STOP
        std::cout << "i=" << i << " mode=" << static_cast<int>(app::mode()) << "\n";

        // Sleep until next update time to maintain consistent timing
        next_tick += duration_cast<steady_clock::duration>(duration<float>(dt));
        std::this_thread::sleep_until(next_tick);
    }
    return 0;
}