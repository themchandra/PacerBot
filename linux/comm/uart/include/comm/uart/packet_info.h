/**
 * @file packet_info.h
 * @brief Contains relevant information about UART data packets
 * @author Hayden Mai
 * @date Oct-29-2025
 */

#ifndef COMM_UART_PACKET_INFO_H_
#define COMM_UART_PACKET_INFO_H_

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

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


    /** @brief Data packet structure for UART communication */
    class DataPacket {
      public:
        // Constructor, meant to be used with storing received packets. Timestamp, sync,
        // and crc8 will be auto-generated.
        DataPacket(ePacketID id, std::span<const uint8_t> data_payload);

        // Convert DataPacket to a uint8_t buffer for UART transmission
        size_t serialize(uint8_t *buf, size_t buf_size) const;

        // Convert UART raw data to DataPacket class only if its valid
        static std::optional<DataPacket> deserialize(const uint8_t *rawData,
                                                     size_t length);

        // Getter methods
        uint8_t getSync() const noexcept { return sync_; }
        ePacketID getID() const noexcept { return id_; }
        uint32_t getTimestamp() const noexcept { return timestamp_; }
        const std::vector<uint8_t> &getData() const noexcept { return data_; }

      private:
        DataPacket(uint8_t sync, ePacketID id, uint32_t timestamp,
                   std::vector<uint8_t> data_payload, uint8_t crc8);

        uint8_t sync_ {};       // Header - 0x5A (Radxa receive) or 0xA5 (Radxa transmit)
        ePacketID id_ {};       // Refer to ePacketID enum class
        uint32_t timestamp_ {}; // milliseconds since mcu boot, big enough for ~49.7 days
        std::vector<uint8_t> data_; // Data
        uint8_t crc8_ {};           // CRC8 checksum

        uint8_t calculate_crc8() const;
        bool validate_crc() const;
        static uint32_t getTimeMs();
    };


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


    // Sync bytes
    constexpr uint8_t SYNC_RECV {0x5A};
    constexpr uint8_t SYNC_SEND {0xA5};

} // namespace uart

#endif