/**
 * @file packet_info.h
 * @brief Contains relevant information about UART data packets
 * @author Hayden Mai
 * @date Nov-03-2025
 */

#ifndef COMM_UART_PACKET_INFO_H_
#define COMM_UART_PACKET_INFO_H_

#include <array>
#include <cstdint>
#include <cstddef>

namespace uart {
    // Max data packet size
    constexpr size_t DATA_MAX_SIZE {255};

    // Sync bytes
    constexpr uint8_t SYNC_RECV {0x5A};
    constexpr uint8_t SYNC_SEND {0xA5};


    /** @brief List of IDs to/from the mcu */
    enum class ePacketID : uint8_t {
        // Receiving (STM32 -> Radxa)
        TELEM_IMU,     // IMU data
        TELEM_ULT,     // Ultrasonic data
        TELEM_ENC,     // Encoder data
        TELEM_PID,     // Contains pid data
        TELEM_BATTERY, // Measured battery voltage
        STM32_STATUS,  // Status of the STM32
        STM32_ACK,     // Confirm receipt from STM32
        STM32_DEBUG,   // Debugging log

        // Transmitting (Radxa -> STM32)
        CMD_MOTOR,      // Motor control
        CMD_NAV,        // Target speed, turn, start/stop
        CONF_PID_SPEED, // Tune speed PID
        CONF_PID_LANE,  // Tune laning PID
        CONF_SENSOR,    // Configure sensor data rate
        RAD_STATUS,     // Status of the Radxa
        RAD_ACK,        // Confirm receipt from Radxa
    };


    /** @brief Raw data packet structure from reading UART */
    struct DataPacket_raw {
        uint8_t sync {};   // Header - 0x5A (Radxa receive) or 0xA5 (Radxa transmit)
        ePacketID id {};   // Refer to ePacketID enum class
        uint8_t length {}; // Max bits length of data: 255 bytes
        uint8_t data[DATA_MAX_SIZE]; // Data
                                     // CRC8 at data[length]

        size_t totalSize() const { return 3 + length + 1; }
    } __attribute__((packed));


    namespace recv {
        /** @brief IMU data structure from the telemetry packet */
        struct IMU_data {
            int16_t accel_x {};
            int16_t accel_y {};
            int16_t accel_z {};
            int16_t gyro_x {};
            int16_t gyro_y {};
            int16_t gyro_z {};
        };
    } // namespace recv


    /** @brief Calculate CRC8 checksum */
    uint8_t calculate_crc8(uint8_t *data, uint16_t length);

} // namespace uart

#endif