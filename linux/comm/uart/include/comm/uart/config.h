/**
 * @file config.h
 * @brief Config constants for UART serial communication
 * @author Hayden Mai
 * @date Nov-06-2025
 */

#ifndef COMM_UART_CONFIG_H_
#define COMM_UART_CONFIG_H_

#include <cstdint>
#include <string>
#include <termios.h>

namespace uart::config {
    // Serial port settings
    const std::string UART_DEVICE {"/dev/ttyS2"};
    constexpr int BAUDRATE {B115200};
    constexpr int TIMEOUT_SEC {1};

	// Queue sizes
    constexpr size_t MAX_TX_QUEUE_SIZE {100};
    constexpr size_t MAX_RX_QUEUE_SIZE {100};

    // Max packet size is 3 (header) + 255 (max data) + 1 (crc8)
    constexpr size_t READ_BUF_SIZE {100};

} // namespace uart::config

#endif