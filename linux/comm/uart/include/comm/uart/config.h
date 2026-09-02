/**
 * @file config.h
 * @brief Config constants for UART serial communication
 * @author Hayden Mai
 * @date Sep-02-2026
 */

#ifndef COMM_UART_CONFIG_H_
#define COMM_UART_CONFIG_H_

#include <cstddef>
#include <cstdint>
#include <string>

namespace uart::config {
    // Serial port settings
    const std::string UART_DEVICE {"/dev/ttyS3"};
    constexpr int BAUDRATE {460800};
    constexpr int TIMEOUT_SEC {10};

    // Queue sizes
    constexpr size_t MAX_TX_QUEUE_SIZE {100};
    constexpr size_t MAX_RX_QUEUE_SIZE {100};

    // Max packet size is 3 (header) + 255 (max data) + 1 (crc8)
    constexpr size_t READ_BUF_SIZE {259};

} // namespace uart::config

#endif