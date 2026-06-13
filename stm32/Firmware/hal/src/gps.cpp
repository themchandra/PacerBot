/**
 * @file gps.cpp
 * @brief GPS class implementation — async DMA u-blox UBX parsing on a single UART
 * @author Hayden Mai
 * @date Jun-13-2026
 */

#include "hal/gps.h"

#include "stm32f4xx_hal.h"

#include <cstdio>

namespace hal {

    namespace {
        constexpr bool kGpsDebug {false};
        constexpr uint32_t kIdleReportMs {2000};
    } // namespace

    GPS::GPS(UART_HandleTypeDef *huart) : huart_(huart)
    {
        dataMutex_ = osMutexNew(nullptr);
    }


    void GPS::start()
    {
        curIdx_ = 0;
        dmaIdx_ = 0;
        resetUbxParser();

        isTaskRunning_ = true;
        taskHandle_    = osThreadNew(taskTrampoline, this, &kTaskAttr);
        if (taskHandle_ == nullptr) {
            isTaskRunning_ = false;
            if (kGpsDebug) {
                std::printf("GPS: failed to create parse task\n\r");
            }
            return;
        }

        const HAL_StatusTypeDef status
            = HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxBuf_, RX_BUF_SIZE);
        if (status != HAL_OK) {
            isTaskRunning_ = false;
            osThreadTerminate(taskHandle_);
            taskHandle_ = nullptr;
            if (kGpsDebug) {
                std::printf("GPS: DMA start failed (status=%d)\n\r",
                            static_cast<int>(status));
            }
            return;
        }

        if (kGpsDebug) {
            std::printf("GPS: UBX parser started on USART%lu @ %lu baud\n\r",
                        static_cast<unsigned long>((huart_->Instance == USART6)   ? 6
                                                   : (huart_->Instance == USART2) ? 2
                                                   : (huart_->Instance == USART1) ? 1
                                                                                  : 0),
                        static_cast<unsigned long>(huart_->Init.BaudRate));
        }
    }


    void GPS::stop()
    {
        isTaskRunning_ = false;
        HAL_UART_DMAStop(huart_);
        osThreadTerminate(taskHandle_);
        taskHandle_ = nullptr;
    }


    bool GPS::isRunning() const { return isTaskRunning_; }


    void GPS::onRxEvent(uint16_t size)
    {
        dmaIdx_ = size;
        rxEventCount_.fetch_add(1, std::memory_order_relaxed);
        osThreadFlagsSet(taskHandle_, FLAGS_VALUE);
    }


    GPS::Data GPS::getData() const
    {
        osMutexAcquire(dataMutex_, osWaitForever);
        const Data snapshot = data_;
        osMutexRelease(dataMutex_);
        return snapshot;
    }


    void GPS::taskTrampoline(void *arg) { static_cast<GPS *>(arg)->taskLoop(); }


    void GPS::taskLoop()
    {
        uint32_t lastActivityMs = osKernelGetTickCount();

        while (isTaskRunning_) {
            const uint32_t flags
                = osThreadFlagsWait(FLAGS_VALUE, osFlagsWaitAny, FLAG_TIMEOUT_MS);

            if (flags != FLAGS_VALUE) {
                if (huart_->hdmarx->State == HAL_DMA_STATE_BUSY) {
                    dmaIdx_ = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart_->hdmarx);
                }
            }

            const uint16_t prevTotalBytes = totalBytesReceived_;
            processBytes(dmaIdx_);

            if (totalBytesReceived_ != prevTotalBytes) {
                lastActivityMs = osKernelGetTickCount();
            } else if (kGpsDebug) {
                const uint32_t now = osKernelGetTickCount();
                if ((now - lastActivityMs) >= kIdleReportMs) {
                    std::printf("GPS: idle — rxEvents=%lu, totalBytes=%u, dmaIdx=%u\n\r",
                                static_cast<unsigned long>(
                                    rxEventCount_.load(std::memory_order_relaxed)),
                                static_cast<unsigned>(totalBytesReceived_),
                                static_cast<unsigned>(dmaIdx_));
                    lastActivityMs = now;
                }
            }
        }
    }


    void GPS::processBytes(uint16_t newEnd)
    {
        uint16_t available {};
        if (curIdx_ <= newEnd) {
            available = newEnd - curIdx_;
        } else {
            available = RX_BUF_SIZE - curIdx_ + newEnd;
        }

        if (available == 0) {
            return;
        }

        totalBytesReceived_ += available;

        if (available > RX_BUF_SIZE) {
            curIdx_ = newEnd;
            return;
        }

        for (uint16_t i = 0; i < available; ++i) {
            processByte(rxBuf_[(curIdx_ + i) % RX_BUF_SIZE]);
        }

        curIdx_ = newEnd % RX_BUF_SIZE;
    }


    void GPS::resetUbxParser()
    {
        ubxState_      = UbxState::Sync1;
        ubxClass_      = 0;
        ubxId_         = 0;
        ubxLen_        = 0;
        ubxPayloadIdx_ = 0;
        ubxCkA_        = 0;
        ubxCkB_        = 0;
        ubxFrameCkA_   = 0;
    }


    void GPS::ubxUpdateCk(uint8_t byte)
    {
        ubxCkA_ = static_cast<uint8_t>(ubxCkA_ + byte);
        ubxCkB_ = static_cast<uint8_t>(ubxCkB_ + ubxCkA_);
    }


    void GPS::processByte(uint8_t byte)
    {
        switch (ubxState_) {
        case UbxState::Sync1:
            if (byte == UBX_SYNC1) {
                ubxState_ = UbxState::Sync2;
            }
            break;

        case UbxState::Sync2:
            if (byte == UBX_SYNC2) {
                ubxCkA_        = 0;
                ubxCkB_        = 0;
                ubxPayloadIdx_ = 0;
                ubxState_      = UbxState::Class;
            } else {
                ubxState_ = (byte == UBX_SYNC1) ? UbxState::Sync2 : UbxState::Sync1;
            }
            break;

        case UbxState::Class:
            ubxClass_ = byte;
            ubxUpdateCk(byte);
            ubxState_ = UbxState::Id;
            break;

        case UbxState::Id:
            ubxId_ = byte;
            ubxUpdateCk(byte);
            ubxState_ = UbxState::LenLo;
            break;

        case UbxState::LenLo:
            ubxLen_ = byte;
            ubxUpdateCk(byte);
            ubxState_ = UbxState::LenHi;
            break;

        case UbxState::LenHi:
            ubxLen_ |= static_cast<uint16_t>(byte) << 8;
            ubxUpdateCk(byte);
            if (ubxLen_ > UBX_MAX_PAYLOAD) {
                resetUbxParser();
                break;
            }
            ubxPayloadIdx_ = 0;
            ubxState_      = (ubxLen_ == 0) ? UbxState::CkA : UbxState::Payload;
            break;

        case UbxState::Payload:
            ubxPayload_[ubxPayloadIdx_++] = byte;
            ubxUpdateCk(byte);
            if (ubxPayloadIdx_ >= ubxLen_) {
                ubxState_ = UbxState::CkA;
            }
            break;

        case UbxState::CkA:
            ubxFrameCkA_ = byte;
            ubxState_    = UbxState::CkB;
            break;

        case UbxState::CkB:
            if (ubxCkA_ == ubxFrameCkA_ && ubxCkB_ == byte) {
                dispatchUbx(ubxClass_, ubxId_, ubxPayload_, ubxLen_);
            }
            resetUbxParser();
            break;
        }
    }


    void GPS::dispatchUbx(uint8_t cls, uint8_t id, const uint8_t *payload, uint16_t len)
    {
        if (cls == UBX_CLASS_NAV && id == UBX_ID_NAV_PVT) {
            parseNavPvt(payload, len);
        }
    }


    int32_t GPS::readI32(const uint8_t *p)
    {
        return static_cast<int32_t>(
            static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
            | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24));
    }


    bool GPS::parseNavPvt(const uint8_t *payload, uint16_t len)
    {
        if (len < UBX_NAV_PVT_MIN_LEN) {
            return false;
        }

        const uint8_t fixType = payload[20];
        const uint8_t flags   = payload[21];
        const uint8_t numSv   = payload[23];
        const int32_t lon     = readI32(payload + 24);
        const int32_t lat     = readI32(payload + 28);
        const int32_t hMsl    = readI32(payload + 36);
        const int32_t gSpeed  = readI32(payload + 60);
        const int32_t headMot = readI32(payload + 64);

        const bool valid = (fixType >= 2) && ((flags & 0x01U) != 0U);

        osMutexAcquire(dataMutex_, osWaitForever);
        data_.lat_deg     = static_cast<float>(lat) * 1.0e-7f;
        data_.lon_deg     = static_cast<float>(lon) * 1.0e-7f;
        data_.alt_m       = static_cast<float>(hMsl) / 1000.0f;
        data_.speed_mps   = static_cast<float>(gSpeed) / 1000.0f;
        data_.heading_deg = static_cast<float>(headMot) * 1.0e-5f;
        data_.sats_used   = numSv;
        data_.fix_qual    = fixType;
        data_.valid       = valid;
        osMutexRelease(dataMutex_);

        if (kGpsDebug) {
            std::printf("GPS: NAV-PVT fix=%u sats=%u valid=%d lat=%.6f lon=%.6f\r\n",
                        static_cast<unsigned>(fixType), static_cast<unsigned>(numSv),
                        static_cast<int>(valid), static_cast<double>(data_.lat_deg),
                        static_cast<double>(data_.lon_deg));
        }

        return true;
    }

} // namespace hal
