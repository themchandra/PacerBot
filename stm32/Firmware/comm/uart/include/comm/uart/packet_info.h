/**
 * @file packet_info.h
 * @brief Contains relevant information about UART data packets
 * @author Hayden Mai
 * @date May-15-2026
 */

#ifndef COMM_UART_PACKET_INFO_H_
#define COMM_UART_PACKET_INFO_H_

#include <array>
#include <cstddef>
#include <cstdint>

namespace uart {
    // Max data packet size
    constexpr size_t DATA_MAX_SIZE {100};

    // Sync bytes
    constexpr uint8_t SYNC_RECV {0xA5};
    constexpr uint8_t SYNC_SEND {0x5A};


    /** @brief List of IDs to/from the mcu */
    enum class ePacketID : uint8_t {
        // Receiving (STM32 -> Host)
        TELEM_IMU,     // IMU data
        TELEM_ULT,     // Ultrasonic data
        TELEM_ENC,     // Encoder data
        TELEM_PID,     // Contains pid data
        TELEM_BATTERY, // Measured battery voltage
        STM32_STATUS,  // Status of the STM32
        STM32_ACK,     // Confirm receipt from STM32
        STM32_DEBUG,   // Debugging log

        // Transmitting (Host -> STM32)
        CMD_TARGET_SPEED, // Target speed command (payload: float)
        TELEM_LINE_POS,   // Line position command (payload: float)
        HOST_STATUS,      // Status of the Host
        HOST_ACK,         // Confirm receipt from Host
        HOST_DEBUG,       // Debugging log from Host

        TOTAL,
    };

    // Packet attributes
    constexpr uint8_t HEADER_SIZE {3};
    constexpr uint8_t CRC_SIZE {1};

    /** @brief Raw data packet structure from reading UART */
    struct DataPacket_raw {
        uint8_t sync {};   // Header - 0x5A (Host receive) or 0xA5 (Host transmit)
        ePacketID id {};   // Refer to ePacketID enum class
        uint8_t length {}; // Max bits length of data array
        uint8_t data[DATA_MAX_SIZE] {}; // Data
                                        // CRC8 at data[length]

        size_t totalSize() const { return HEADER_SIZE + length + CRC_SIZE; }
    } __attribute__((packed));


    /** @brief Calculate CRC8 checksum. */
    uint8_t calculate_crc8(const uint8_t *data, uint16_t length, uint8_t crc = 0x00);

} // namespace uart

#endif