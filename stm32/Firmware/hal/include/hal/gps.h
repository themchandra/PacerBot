/**
 * @file gps.h
 * @brief GPS class for async DMA u-blox UBX parsing
 * @author Hayden Mai
 * @date Jun-15-2026
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
            // Time
            uint32_t itow_ms {};
            uint16_t year {};
            uint8_t month {};
            uint8_t day {};
            uint8_t hour {};
            uint8_t min {};
            uint8_t sec {};
            uint8_t datetime_valid {};
            uint32_t t_acc_ns {};
            int32_t nano_ns {};

            // Fix / flags
            uint8_t fix_qual {};
            uint8_t flags {};
            uint8_t flags2 {};
            uint8_t flags3 {};
            uint8_t sats_used {};
            bool valid {};

            // Position
            float lat_deg {};
            float lon_deg {};
            float height_m {};
            float alt_m {};
            float h_acc_m {};
            float v_acc_m {};

            // Velocity
            float vel_n_mps {};
            float vel_e_mps {};
            float vel_d_mps {};
            float speed_mps {};
            float heading_deg {};
            float s_acc_mps {};
            float head_acc_deg {};
            float p_dop {};

            // Vehicle / magnetic
            float head_veh_deg {};
            float mag_dec_deg {};
            float mag_acc_deg {};
        };

        explicit GPS(UART_HandleTypeDef *huart);

        /**
         * @brief Return the UART handle this GPS instance is bound to.
         * @note Used by the UART callback router to identify the correct instance.
         */
        UART_HandleTypeDef *getHuart() const { return huart_; }

        /**
         * @brief Start DMA receive and spawn the parse task.
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
        static constexpr uint16_t UBX_MAX_PAYLOAD {256};

        static constexpr uint32_t STACK_BYTES {2048};
        static constexpr uint32_t FLAGS_VALUE {0x01};
        static constexpr uint32_t FLAG_TIMEOUT_MS {10};

        static constexpr uint8_t UBX_SYNC1 {0xB5};
        static constexpr uint8_t UBX_SYNC2 {0x62};
        static constexpr uint8_t UBX_CLASS_NAV {0x01};
        static constexpr uint8_t UBX_ID_NAV_PVT {0x07};
        static constexpr uint16_t UBX_NAV_PVT_MIN_LEN {92};

        enum class UbxState : uint8_t {
            Sync1,
            Sync2,
            Class,
            Id,
            LenLo,
            LenHi,
            Payload,
            CkA,
            CkB,
        };

        static constexpr osThreadAttr_t task_attr_ {
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

        UART_HandleTypeDef *huart_;
        uint8_t rxBuf_[RX_BUF_SIZE] {};
        uint16_t curIdx_ {};
        uint16_t dmaIdx_ {};

        UbxState ubxState_ {UbxState::Sync1};
        uint8_t ubxClass_ {};
        uint8_t ubxId_ {};
        uint16_t ubxLen_ {};
        uint16_t ubxPayloadIdx_ {};
        uint8_t ubxPayload_[UBX_MAX_PAYLOAD] {};
        uint8_t ubxCkA_ {};
        uint8_t ubxCkB_ {};
        uint8_t ubxFrameCkA_ {};

        Data data_ {};
        osMutexId_t dataMutex_ {};

        bool isTaskRunning_ {false};
        osThreadId_t taskHandle_ {nullptr};

        static void taskTrampoline(void *arg);
        void taskLoop();
        void processBytes(uint16_t newEnd);
        void processByte(uint8_t byte);
        void ubxUpdateCk(uint8_t byte);
        void resetUbxParser();
        void dispatchUbx(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len);
        bool parseNavPvt(const uint8_t *payload, uint16_t len);
        static int32_t read_int32(const uint8_t *p);
        static uint32_t read_uint32(const uint8_t *p);
        static uint16_t read_uint16(const uint8_t *p);
        static int16_t read_int16(const uint8_t *p);
    };

} // namespace hal

#endif // HAL_GPS_H_
