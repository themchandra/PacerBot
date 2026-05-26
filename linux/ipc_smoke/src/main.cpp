#include "comm/ipc/lane_pipe_reader.h"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
    volatile std::sig_atomic_t g_running = 1;

    void handleSignal(int)
    {
        g_running = 0;
    }
} // namespace

int main()
{
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    std::cout << "IPC smoke test started. Reading /tmp/pacerbot_lane.pipe\n";
    std::cout << "Start the lane detection writer in another terminal.\n";

    LaneInput input {};
    while (g_running) {
        if (readLaneInput(input)) {
            std::cout << "LaneInput{valid=" << std::boolalpha << input.valid
                      << ", steering_error=" << input.steering_error << "}\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::cout << "IPC smoke test stopped.\n";
    return EXIT_SUCCESS;
}
