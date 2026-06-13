/**
 * @file gps.h
 * @brief GPS class for async DMA NMEA parsing
 * @author Hayden Mai
 * @date Jun-13-2026
 */

#ifndef HAL_GPS_H_
#define HAL_GPS_H_

#include "cmsis_os.h"
#include "main.h"

#include <cstdint>

namespace hal {

    class GPS {
      public:
        struct Data {
            float lat_deg {};
            float lon_deg {};
            float alt_m {};
            float speed_mps {};
            float heading_deg {};
            uint8_t sats_used {};
            uint8_t fix_qual {};
            bool valid {};
        };

        explicit GPS(UART_HandleTypeDef *huart);

        /**
         * @brief Return the UART handle this GPS instance is bound to.
         * @note Used by the UART callback router to identify the correct instance.
         */
        UART_HandleTypeDef *getHuart() const { return huart_; }

        /**
         * @brief Start DMA receive and spawn the parse task.
         * @note Resets buffer indices and line accumulator before starting.
         */
        void start();

        /**
         * @brief Stop DMA receive and terminate the parse task.
         */
        void stop();

        /**
         * @brief Return whether the parse task is currently running.
         */
        bool isRunning() const;

        /**
         * @brief Update the DMA write position and wake the parse task.
         * @note Must be called from HAL_UARTEx_RxEventCallback — ISR context.
         * @param size Number of bytes received into the DMA buffer so far.
         */
        void onRxEvent(uint16_t size);

        /**
         * @brief Return a snapshot of the latest parsed GPS data.
         * @note Thread-safe; acquires the internal mutex.
         */
        Data getData() const;

      private:
        static constexpr uint16_t RX_BUF_SIZE {512};
        static constexpr uint16_t LINE_BUF_SIZE {128};
        static constexpr uint32_t STACK_BYTES {2048};
        static constexpr uint32_t FLAGS_VALUE {0x01};
        static constexpr uint32_t FLAG_TIMEOUT_MS {10};

        static constexpr osThreadAttr_t kTaskAttr {
            .name       = "gpsTask",
            .attr_bits  = 0,
            .cb_mem     = nullptr,
            .cb_size    = 0,
            .stack_mem  = nullptr,
            .stack_size = STACK_BYTES,
            .priority   = osPriorityBelowNormal,
            .tz_module  = 0,
            .reserved   = 0,
        };

        // DMA circular buffer
        UART_HandleTypeDef *huart_;
        uint8_t rxBuf_[RX_BUF_SIZE] {};
        uint16_t curIdx_ {}; // parse head, updated by task
        uint16_t dmaIdx_ {}; // DMA write position, updated by onRxEvent

        // NMEA line accumulator
        char lineBuf_[LINE_BUF_SIZE] {};
        uint16_t lineLen_ {};

        // Latest parsed data, guarded by mutex
        Data data_ {};
        osMutexId_t dataMutex_ {};

        // FreeRTOS task
        bool isTaskRunning_ {false};
        osThreadId_t taskHandle_ {nullptr};

        static void taskTrampoline(void *arg);
        void taskLoop();

        /**
         * @brief Walk newly received DMA bytes into the NMEA line accumulator.
         * @param newEnd Current DMA write position in rxBuf_.
         */
        void processBytes(uint16_t newEnd);

        /**
         * @brief Verify the NMEA checksum of the current line and dispatch to
         *        the appropriate sentence parser.
         */
        void parseLine();

        /**
         * @brief Parse a GGA sentence and update lat, lon, alt, fix_qual, sats_used.
         * @param s Null-terminated GGA sentence string.
         * @return true if all required fields were present and parsed.
         */
        bool parseGGA(const char *s);

        /**
         * @brief Parse an RMC sentence and update valid, lat, lon, speed_mps,
         *        heading_deg.
         * @param s Null-terminated RMC sentence string.
         * @return true if all required fields were present and parsed.
         */
        bool parseRMC(const char *s);

        /**
         * @brief Extract the Nth comma-separated field from an NMEA sentence.
         * @param sentence Null-terminated NMEA sentence string.
         * @param fieldNum 1-based field index.
         * @param out Output buffer.
         * @param outLen Size of the output buffer including the null terminator.
         * @return true if a non-empty field was found, false otherwise.
         */
        static bool getField(const char *sentence, int fieldNum, char *out,
                             size_t outLen);

        /**
         * @brief Convert an NMEA coordinate string to decimal degrees.
         * @param field NMEA coordinate in DDMM.mmmm or DDDMM.mmmm format.
         * @param degDigits Number of integer degree digits: 2 for latitude, 3 for
         * longitude.
         * @return Decimal degrees (always positive — apply N/S or E/W sign after).
         */
        static float nmeaToDecimalDeg(const char *field, int degDigits);
    };

} // namespace hal

#endif // HAL_GPS_H_
