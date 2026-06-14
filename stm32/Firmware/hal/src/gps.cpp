/**
 * @file gps.cpp
 * @brief GPS class implementation — async DMA u-blox UBX parsing on a single UART
 * @author Hayden Mai
 * @date Jun-14-2026
 */

#include "hal/gps.h"

#include "stm32f4xx_hal.h"

namespace hal {

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
        taskHandle_    = osThreadNew(taskTrampoline, this, &task_attr_);
        if (taskHandle_ == nullptr) {
            isTaskRunning_ = false;
            return;
        }

        const HAL_StatusTypeDef status
            = HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxBuf_, RX_BUF_SIZE);
        if (status != HAL_OK) {
            isTaskRunning_ = false;
            osThreadTerminate(taskHandle_);
            taskHandle_ = nullptr;
            return;
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
        while (isTaskRunning_) {
            const uint32_t flags
                = osThreadFlagsWait(FLAGS_VALUE, osFlagsWaitAny, FLAG_TIMEOUT_MS);

            if (flags != FLAGS_VALUE) {
                if (huart_->hdmarx->State == HAL_DMA_STATE_BUSY) {
                    dmaIdx_ = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart_->hdmarx);
                }
            }

            processBytes(dmaIdx_);
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


    int32_t GPS::read_int32(const uint8_t *p)
    {
        return static_cast<int32_t>(
            static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
            | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24));
    }


    uint32_t GPS::read_uint32(const uint8_t *p)
    {
        return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
             | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
    }


    uint16_t GPS::read_uint16(const uint8_t *p)
    {
        return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
    }


    int16_t GPS::read_int16(const uint8_t *p)
    {
        return static_cast<int16_t>(read_uint16(p));
    }


    bool GPS::parseNavPvt(const uint8_t *payload, uint16_t len)
    {
        if (len < UBX_NAV_PVT_MIN_LEN) {
            return false;
        }

        const uint32_t itow    = read_uint32(payload + 0);
        const uint16_t year    = read_uint16(payload + 4);
        const uint8_t fixType  = payload[20];
        const uint8_t flags    = payload[21];
        const uint8_t flags2   = payload[22];
        const uint8_t numSv    = payload[23];
        const int32_t lon      = read_int32(payload + 24);
        const int32_t lat      = read_int32(payload + 28);
        const int32_t height   = read_int32(payload + 32);
        const int32_t hMsl     = read_int32(payload + 36);
        const uint32_t hAcc    = read_uint32(payload + 40);
        const uint32_t vAcc    = read_uint32(payload + 44);
        const int32_t velN     = read_int32(payload + 48);
        const int32_t velE     = read_int32(payload + 52);
        const int32_t velD     = read_int32(payload + 56);
        const int32_t gSpeed   = read_int32(payload + 60);
        const int32_t headMot  = read_int32(payload + 64);
        const uint32_t sAcc    = read_uint32(payload + 68);
        const uint32_t headAcc = read_uint32(payload + 72);
        const uint16_t pDop    = read_uint16(payload + 76);
        const uint8_t flags3   = payload[78];
        const int32_t headVeh  = read_int32(payload + 86);
        const int16_t magDec   = read_int16(payload + 90);
        const uint16_t magAcc  = (len >= 94U) ? read_uint16(payload + 92) : 0U;

        const bool valid = (fixType >= 2) && ((flags & 0x01U) != 0U);

        osMutexAcquire(dataMutex_, osWaitForever);
        data_.itow_ms        = itow;
        data_.year           = year;
        data_.month          = payload[6];
        data_.day            = payload[7];
        data_.hour           = payload[8];
        data_.min            = payload[9];
        data_.sec            = payload[10];
        data_.datetime_valid = payload[11];
        data_.t_acc_ns       = read_uint32(payload + 12);
        data_.nano_ns        = read_int32(payload + 16);
        data_.fix_qual       = fixType;
        data_.flags          = flags;
        data_.flags2         = flags2;
        data_.flags3         = flags3;
        data_.sats_used      = numSv;
        data_.valid          = valid;
        data_.lat_deg        = static_cast<float>(lat) * 1.0e-7f;
        data_.lon_deg        = static_cast<float>(lon) * 1.0e-7f;
        data_.height_m       = static_cast<float>(height) / 1000.0f;
        data_.alt_m          = static_cast<float>(hMsl) / 1000.0f;
        data_.h_acc_m        = static_cast<float>(hAcc) / 1000.0f;
        data_.v_acc_m        = static_cast<float>(vAcc) / 1000.0f;
        data_.vel_n_mps      = static_cast<float>(velN) / 1000.0f;
        data_.vel_e_mps      = static_cast<float>(velE) / 1000.0f;
        data_.vel_d_mps      = static_cast<float>(velD) / 1000.0f;
        data_.speed_mps      = static_cast<float>(gSpeed) / 1000.0f;
        data_.heading_deg    = static_cast<float>(headMot) * 1.0e-5f;
        data_.s_acc_mps      = static_cast<float>(sAcc) / 1000.0f;
        data_.head_acc_deg   = static_cast<float>(headAcc) * 1.0e-5f;
        data_.p_dop          = static_cast<float>(pDop) * 0.01f;
        data_.head_veh_deg   = static_cast<float>(headVeh) * 1.0e-5f;
        data_.mag_dec_deg    = static_cast<float>(magDec) * 0.01f;
        data_.mag_acc_deg    = static_cast<float>(magAcc) * 0.01f;
        osMutexRelease(dataMutex_);

        return true;
    }

} // namespace hal
