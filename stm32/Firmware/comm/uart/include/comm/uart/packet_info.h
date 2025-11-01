/**
 * @file packet_info.h
 * @brief Contains relevant information about UART data packets
 * @author Hayden Mai
 * @date Oct-31-2025
 */

#ifndef COMM_UART_PACKET_INFO_H_
#define COMM_UART_PACKET_INFO_H_

#include <array>
#include <cstdint>

namespace uart {
    /** @brief List of IDs to/from the mcu */
    enum class ePacketID : uint8_t {
        // Receiving (STM32 -> Radxa)
        TELEMETRY,    // Contains sensor (imu, ultrasonic, encoder), pid(s) data
        STATUS_STM32, // Status of the STM32
        BATTERY,      // Measured battery voltage
        ACK_STM32,    // Confirm receipt from STM32
        DEBUG,        // Debugging log

        // Transmitting (Radxa -> STM32)
        CMD_MOTOR,        // Motor control
        CMD_NAV,          // Target speed, turn, start/stop
        CONFIG_PID_SPEED, // Tune speed PID
        CONFIG_PID_LANE,  // Tune laning PID
        CONFIG_SENSOR,    // Configure sensor data rate
        STATUS_RADXA,     // Status of the Radxa
        ACK_RADXA,        // Confirm receipt from Radxa
    };


    // Max data packet size
    constexpr size_t DATA_MAX_SIZE {256};

    // Sync bytes
    constexpr uint8_t SYNC_RECV {0x5A};
    constexpr uint8_t SYNC_SEND {0xA5};

    /** @brief Raw data packet structure from reading UART */
    struct DataPacket_raw {
        uint8_t sync {};       // Header - 0x5A (Radxa receive) or 0xA5 (Radxa transmit)
        ePacketID id {};       // Refer to ePacketID enum class
        uint32_t timestamp {}; // millisecs since program start, big enough for ~49.7 days
        uint8_t length {};     // Max bits length of data: 255 bytes
        uint8_t data[DATA_MAX_SIZE]; // Data
                                     // CRC8 at data[length]

        size_t totalSize() const { return 7 + length + 1; }
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

} // namespace uart

#endif