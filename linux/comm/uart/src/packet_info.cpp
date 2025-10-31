/**
 * @file packet_info.cpp
 * @brief Contains relevant information about UART data packets
 * @author Hayden Mai
 * @date Oct-30-2025
 */

#include "comm/uart/config.h"
#include "comm/uart/packet_info.h"

#include <chrono>
#include <iostream>
#include <string.h>

namespace uart {
    // Initialized when program starts
    const auto start_time = std::chrono::steady_clock::now();

    DataPacket::DataPacket(ePacketID id, std::span<const uint8_t> data_payload)
        : sync_(SYNC_RECV),
          id_(id),
          timestamp_(getTimeMs()),
          data_(data_payload.begin(), data_payload.end())
    {
        crc8_ = calculate_crc8();
    }


    DataPacket::DataPacket(uint8_t sync, ePacketID id, uint32_t timestamp,
                           std::vector<uint8_t> data_payload, uint8_t crc8)
        : sync_(sync),
          id_(id),
          timestamp_(timestamp),
          data_(std::move(data_payload)),
          crc8_(crc8)
    {}


    size_t DataPacket::serialize(uint8_t *buf, size_t buf_size) const
    {
        size_t packet_size {sizeof(DataPacket_raw) + data_.size() + 1};
        if (buf_size < packet_size) {
            return 0;
        }

        DataPacket_raw *raw_ptr = reinterpret_cast<DataPacket_raw *>(buf);
        raw_ptr->sync           = sync_;
        raw_ptr->id             = id_;
        raw_ptr->timestamp      = timestamp_;
        raw_ptr->length         = static_cast<uint8_t>(data_.size());
        memcpy(raw_ptr->data, data_.data(), data_.size());
        raw_ptr->data[raw_ptr->length] = crc8_;

        return packet_size;
    }


    std::optional<DataPacket> DataPacket::deserialize(const uint8_t *rawData,
                                                      size_t length)
    {
        if (!rawData || length > sizeof(DataPacket_raw) || length == 0) {
            std::cout << "Invalid packet\n";
            return std::nullopt;
        }

        // Reinterpret bytes as a DataPacket_raw struct
        const DataPacket_raw *raw_ptr = reinterpret_cast<const DataPacket_raw *>(rawData);

        // Validate sync byte
        if (raw_ptr->sync != 0x5A && raw_ptr->sync != 0xA5) {
            std::cout << "Invalid sync byte\n";
            return std::nullopt;
        }

        // Check raw packet length, +1 for crc8
        size_t expected_length {raw_ptr->totalSize()};
        if (length < expected_length) {
            std::cout << "Invalid length\n";
            return std::nullopt;
        }

        // Extract data
        std::vector<uint8_t> data(raw_ptr->data, raw_ptr->data + raw_ptr->length);
        uint8_t raw_crc8 {raw_ptr->data[raw_ptr->length]};

        // Store data & validate checksum
        DataPacket packet(raw_ptr->sync, raw_ptr->id, raw_ptr->timestamp, std::move(data),
                          raw_crc8);
        if (!packet.validate_crc()) {
            std::cout << "Invalid checksum\n";
            return std::nullopt;
        }

        return packet;
    }


    uint8_t DataPacket::calculate_crc8() const
    {
        // Using CRC8 Opensafety (0x2F)
        constexpr uint8_t CRC8_TABLE[256] = {
            0x00, 0x2F, 0x5E, 0x71, 0xBC, 0x93, 0xE2, 0xCD, 0x57, 0x78, 0x09, 0x26, 0xEB,
            0xC4, 0xB5, 0x9A, 0xAE, 0x81, 0xF0, 0xDF, 0x12, 0x3D, 0x4C, 0x63, 0xF9, 0xD6,
            0xA7, 0x88, 0x45, 0x6A, 0x1B, 0x34, 0x73, 0x5C, 0x2D, 0x02, 0xCF, 0xE0, 0x91,
            0xBE, 0x24, 0x0B, 0x7A, 0x55, 0x98, 0xB7, 0xC6, 0xE9, 0xDD, 0xF2, 0x83, 0xAC,
            0x61, 0x4E, 0x3F, 0x10, 0x8A, 0xA5, 0xD4, 0xFB, 0x36, 0x19, 0x68, 0x47, 0xE6,
            0xC9, 0xB8, 0x97, 0x5A, 0x75, 0x04, 0x2B, 0xB1, 0x9E, 0xEF, 0xC0, 0x0D, 0x22,
            0x53, 0x7C, 0x48, 0x67, 0x16, 0x39, 0xF4, 0xDB, 0xAA, 0x85, 0x1F, 0x30, 0x41,
            0x6E, 0xA3, 0x8C, 0xFD, 0xD2, 0x95, 0xBA, 0xCB, 0xE4, 0x29, 0x06, 0x77, 0x58,
            0xC2, 0xED, 0x9C, 0xB3, 0x7E, 0x51, 0x20, 0x0F, 0x3B, 0x14, 0x65, 0x4A, 0x87,
            0xA8, 0xD9, 0xF6, 0x6C, 0x43, 0x32, 0x1D, 0xD0, 0xFF, 0x8E, 0xA1, 0xE3, 0xCC,
            0xBD, 0x92, 0x5F, 0x70, 0x01, 0x2E, 0xB4, 0x9B, 0xEA, 0xC5, 0x08, 0x27, 0x56,
            0x79, 0x4D, 0x62, 0x13, 0x3C, 0xF1, 0xDE, 0xAF, 0x80, 0x1A, 0x35, 0x44, 0x6B,
            0xA6, 0x89, 0xF8, 0xD7, 0x90, 0xBF, 0xCE, 0xE1, 0x2C, 0x03, 0x72, 0x5D, 0xC7,
            0xE8, 0x99, 0xB6, 0x7B, 0x54, 0x25, 0x0A, 0x3E, 0x11, 0x60, 0x4F, 0x82, 0xAD,
            0xDC, 0xF3, 0x69, 0x46, 0x37, 0x18, 0xD5, 0xFA, 0x8B, 0xA4, 0x05, 0x2A, 0x5B,
            0x74, 0xB9, 0x96, 0xE7, 0xC8, 0x52, 0x7D, 0x0C, 0x23, 0xEE, 0xC1, 0xB0, 0x9F,
            0xAB, 0x84, 0xF5, 0xDA, 0x17, 0x38, 0x49, 0x66, 0xFC, 0xD3, 0xA2, 0x8D, 0x40,
            0x6F, 0x1E, 0x31, 0x76, 0x59, 0x28, 0x07, 0xCA, 0xE5, 0x94, 0xBB, 0x21, 0x0E,
            0x7F, 0x50, 0x9D, 0xB2, 0xC3, 0xEC, 0xD8, 0xF7, 0x86, 0xA9, 0x64, 0x4B, 0x3A,
            0x15, 0x8F, 0xA0, 0xD1, 0xFE, 0x33, 0x1C, 0x6D, 0x42};
        constexpr uint8_t CRC_INIT {0x00};

        uint8_t crc8 {CRC_INIT};
        crc8 = CRC8_TABLE[crc8 ^ sync_];
        crc8 = CRC8_TABLE[crc8 ^ static_cast<uint8_t>(id_)];

        // Timestamp
        crc8 = CRC8_TABLE[crc8 ^ static_cast<uint8_t>(timestamp_ & 0xFF)];
        crc8 = CRC8_TABLE[crc8 ^ static_cast<uint8_t>((timestamp_ >> 8) & 0xFF)];
        crc8 = CRC8_TABLE[crc8 ^ static_cast<uint8_t>((timestamp_ >> 16) & 0xFF)];
        crc8 = CRC8_TABLE[crc8 ^ static_cast<uint8_t>((timestamp_ >> 24) & 0xFF)];

        crc8 = CRC8_TABLE[crc8 ^ static_cast<uint8_t>(data_.size())];

        // Loop over every byte in data_
        for (uint8_t i : data_) {
            crc8 = CRC8_TABLE[crc8 ^ i];
        }

        return crc8;
    }


    bool DataPacket::validate_crc() const
    {
        uint8_t computed_crc8 = calculate_crc8();
        return computed_crc8 == crc8_;
    }


    uint32_t DataPacket::getTimeMs()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed
            = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        return static_cast<uint32_t>(elapsed.count());
    }

} // namespace uart