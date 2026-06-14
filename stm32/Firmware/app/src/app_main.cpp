/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date May-15-2026
 */

#include "app/app_main.h"

#include "cmsis_os.h"
#include "hal/gps.h"
#include "main.h"

#include <cstdio>

extern "C" {
extern UART_HandleTypeDef huart6;
}

namespace {
    hal::GPS gps(&huart6);

    constexpr uint32_t LED_BLINK_MS {500};

    StaticTask_t ledTaskCb_;
    StackType_t ledTaskStack_[128];

    constexpr osThreadAttr_t kLedTaskAttr {
        .name       = "ledBlink",
        .attr_bits  = 0,
        .cb_mem     = &ledTaskCb_,
        .cb_size    = sizeof(ledTaskCb_),
        .stack_mem  = ledTaskStack_,
        .stack_size = sizeof(ledTaskStack_),
        .priority   = osPriorityLow,
        .tz_module  = 0,
        .reserved   = 0,
    };

    void ledBlinkTask(void *)
    {
        while (true) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            osDelay(LED_BLINK_MS);
        }
    }
} // namespace

// Route DMA idle-line events to the GPS instance
extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart->Instance == USART6) {
        gps.onRxEvent(size);
    }
}

extern "C" void app_main(void)
{
    osThreadNew(ledBlinkTask, nullptr, &kLedTaskAttr);

    gps.start();

    while (true) {
        osDelay(1000);

        const hal::GPS::Data d = gps.getData();
        if (d.valid) {
            std::printf("GPS fix | lat=%.6f lon=%.6f alt=%.1fm spd=%.2fm/s hdg=%.1f sats=%u qual=%u\r\n",
                        static_cast<double>(d.lat_deg),
                        static_cast<double>(d.lon_deg),
                        static_cast<double>(d.alt_m),
                        static_cast<double>(d.speed_mps),
                        static_cast<double>(d.heading_deg),
                        d.sats_used,
                        d.fix_qual);
        } else {
            std::printf("GPS no fix | sats=%u qual=%u\r\n", d.sats_used, d.fix_qual);
        }
    }
}
