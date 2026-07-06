#include "hal/encoders.h"
#include "hal/motors.h"
#include "state_machine.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>

#ifdef PACERBOT_HAS_UART
#include "comm/uart/manager.h"
#endif

#include "timing.h"

char getChar()
{
    // Store character read from the keyboard
    char buf = 0;

    // Save current terminal settings
    termios old;
    if (tcgetattr(STDIN_FILENO, &old) < 0) {
        perror("tcgetattr()");
        return '\0';
    }

    termios current = old;

    // Turn off canonical mode (no enter needed)
    // and echo mode (don't display typed characters)
    current.c_lflag &= ~ICANON;
    current.c_lflag &= ~ECHO;

    // min characters to read
    current.c_cc[VMIN] = 1;

    // no timeout
    current.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &current) < 0) {
        perror("tcsetattr ICANON");
        return '\0';
    }
    // read character
    if (read(STDIN_FILENO, &buf, 1) < 0) {
        perror("read()");
    }
    // restore original terminal settings
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, &old) < 0)
        perror("restore terminal");

    return buf;
}

void test()
{
#ifdef PACERBOT_HAS_UART
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

        auto ack_packet
            = uart::DataPacket(uart::ePacketID::HOST_ACK, std::span<const uint8_t>());
        uart::send::enqueue(std::move(ack_packet));
        timing::sleepForMs(100);
    }
#endif
}

/**
 * Main demonstration program for PacerBot state machine
 * This program runs a simple sequence to test the different states
 */
int main()
{
    while (1) {

        std::cout << getChar() << '\n';
    }
    return 0;
    /*
    uart::manager::init();
    uart::manager::start();

    timing::init();

    std::thread thread_(test);

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
    */
}
