/**
 * @file gps.cpp
 * @brief GPS class implementation — async DMA NMEA parsing on a single UART
 * @author Hayden Mai
 * @date Jun-12-2026
 */

#include "hal/gps.h"

#include "stm32f4xx_hal.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace hal {

    /**
     * @brief Extract the Nth comma-separated field from an NMEA sentence.
     * @param sentence Null-terminated NMEA sentence string.
     * @param fieldNum 1-based field index (field 1 is the first value after the
     *                 sentence ID).
     * @param out Output buffer.
     * @param outLen Size of the output buffer including the null terminator.
     * @return true if a non-empty field was found, false otherwise.
     */
    bool GPS::getField(const char *sentence, int fieldNum, char *out, size_t outLen)
    {
        const char *p = sentence;
        int field     = 0;

        while (*p != '\0') {
            if (*p == ',') {
                ++field;
                ++p;
                if (field == fieldNum) {
                    size_t i = 0;
                    while (*p != ',' && *p != '*' && *p != '\0' && i < outLen - 1) {
                        out[i++] = *p++;
                    }
                    out[i] = '\0';
                    return i > 0;
                }
            } else {
                ++p;
            }
        }

        out[0] = '\0';
        return false;
    }


    double GPS::nmeaToDecimalDeg(const char *field, int degDigits)
    {
        const double raw = atof(field);
        const int deg    = static_cast<int>(raw / 100.0);
        const double min = raw - deg * 100.0;
        (void)degDigits;
        return deg + min / 60.0;
    }

    GPS::GPS(UART_HandleTypeDef *huart) : huart_(huart)
    {
        dataMutex_ = osMutexNew(nullptr);
    }


    void GPS::start()
    {
        curIdx_  = 0;
        dmaIdx_  = 0;
        lineLen_ = 0;

        const HAL_StatusTypeDef status
            = HAL_UARTEx_ReceiveToIdle_DMA(huart_, rxBuf_, RX_BUF_SIZE);
        if (status != HAL_OK) {
            printf("GPS: failed to start DMA receive (status=%d)\n\r", status);
            return;
        }

        isTaskRunning_ = true;
        taskHandle_    = osThreadNew(taskTrampoline, this, &kTaskAttr);
        if (taskHandle_ == nullptr) {
            isTaskRunning_ = false;
            HAL_UART_DMAStop(huart_);
            printf("GPS: failed to create task\n\r");
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

            // On timeout, poll DMA counter directly
            if (flags != FLAGS_VALUE) {
                dmaIdx_ = RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart_->hdmarx);
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

        if (available >= RX_BUF_SIZE) {
            curIdx_ = newEnd;
            printf("GPS: buffer overflow\n\r");
            return;
        }

        for (uint16_t i = 0; i < available; ++i) {
            const char c = static_cast<char>(rxBuf_[(curIdx_ + i) % RX_BUF_SIZE]);

            if (c == '\n') {
                lineBuf_[lineLen_] = '\0';
                if (lineLen_ > 0) {
                    parseLine();
                }
                lineLen_ = 0;
            } else if (c != '\r' && lineLen_ < LINE_BUF_SIZE - 1) {
                lineBuf_[lineLen_++] = c;
            }
        }

        curIdx_ = newEnd;
    }


    void GPS::parseLine()
    {
        // Locate the checksum delimiter
        char *star = strchr(lineBuf_, '*');
        if (star == nullptr) {
            return;
        }

        // Verify XOR checksum over characters between '$' and '*' (exclusive)
        uint8_t calc = 0;
        for (const char *p = lineBuf_ + 1; p < star; ++p) {
            calc ^= static_cast<uint8_t>(*p);
        }

        const uint8_t recv = static_cast<uint8_t>(strtoul(star + 1, nullptr, 16));
        if (calc != recv) {
            return;
        }

        if (strncmp(lineBuf_, "$GNGGA", 6) == 0 || strncmp(lineBuf_, "$GPGGA", 6) == 0) {
            parseGGA(lineBuf_);
        } else if (strncmp(lineBuf_, "$GNRMC", 6) == 0
                   || strncmp(lineBuf_, "$GPRMC", 6) == 0) {
            parseRMC(lineBuf_);
        }
    }


    bool GPS::parseGGA(const char *s)
    {
        char latStr[16] {}, nsStr[4] {};
        char lonStr[16] {}, ewStr[4] {};
        char fixStr[4] {}, satStr[4] {};
        char altStr[16] {};

        // GGA: $--GGA,f1=time,f2=lat,f3=N/S,f4=lon,f5=E/W,f6=fix,f7=sats,...,f9=alt,...
        if (!getField(s, 2, latStr, sizeof(latStr))) {
            return false;
        }
        if (!getField(s, 3, nsStr, sizeof(nsStr))) {
            return false;
        }
        if (!getField(s, 4, lonStr, sizeof(lonStr))) {
            return false;
        }
        if (!getField(s, 5, ewStr, sizeof(ewStr))) {
            return false;
        }
        if (!getField(s, 6, fixStr, sizeof(fixStr))) {
            return false;
        }
        if (!getField(s, 7, satStr, sizeof(satStr))) {
            return false;
        }
        getField(s, 9, altStr, sizeof(altStr)); // altitude can be absent

        double lat = nmeaToDecimalDeg(latStr, 2);
        if (nsStr[0] == 'S') {
            lat = -lat;
        }

        double lon = nmeaToDecimalDeg(lonStr, 3);
        if (ewStr[0] == 'W') {
            lon = -lon;
        }

        const uint8_t fixQual  = static_cast<uint8_t>(atoi(fixStr));
        const uint8_t satsUsed = static_cast<uint8_t>(atoi(satStr));
        const double altM      = atof(altStr);

        osMutexAcquire(dataMutex_, osWaitForever);
        data_.lat_deg   = lat;
        data_.lon_deg   = lon;
        data_.alt_m     = altM;
        data_.fix_qual  = fixQual;
        data_.sats_used = satsUsed;
        osMutexRelease(dataMutex_);

        return true;
    }


    bool GPS::parseRMC(const char *s)
    {
        char statusStr[4] {};
        char latStr[16] {}, nsStr[4] {};
        char lonStr[16] {}, ewStr[4] {};
        char speedStr[16] {};
        char courseStr[16] {};

        // RMC:
        // $--RMC,f1=time,f2=A/V,f3=lat,f4=N/S,f5=lon,f6=E/W,f7=speed_kn,f8=course,...
        if (!getField(s, 2, statusStr, sizeof(statusStr))) {
            return false;
        }
        if (!getField(s, 3, latStr, sizeof(latStr))) {
            return false;
        }
        if (!getField(s, 4, nsStr, sizeof(nsStr))) {
            return false;
        }
        if (!getField(s, 5, lonStr, sizeof(lonStr))) {
            return false;
        }
        if (!getField(s, 6, ewStr, sizeof(ewStr))) {
            return false;
        }
        if (!getField(s, 7, speedStr, sizeof(speedStr))) {
            return false;
        }
        getField(s, 8, courseStr, sizeof(courseStr)); // heading absent when stationary

        const bool valid = (statusStr[0] == 'A');

        double lat = nmeaToDecimalDeg(latStr, 2);
        if (nsStr[0] == 'S') {
            lat = -lat;
        }

        double lon = nmeaToDecimalDeg(lonStr, 3);
        if (ewStr[0] == 'W') {
            lon = -lon;
        }

        const double speedMps   = atof(speedStr) * 0.514444;
        const double headingDeg = atof(courseStr);

        osMutexAcquire(dataMutex_, osWaitForever);
        data_.valid       = valid;
        data_.lat_deg     = lat;
        data_.lon_deg     = lon;
        data_.speed_mps   = speedMps;
        data_.heading_deg = headingDeg;
        osMutexRelease(dataMutex_);

        return true;
    }

} // namespace hal
