/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Jun-13-2026
 */

#include "app/app_main.h"
#include "comm/uart/manager.h"
#include "comm/uart/recv.h"

#include "cmsis_os.h"
#include "hal/gps.h"
#include "main.h"

#include <cstdio>

/*
 * ESC Deadzones
 * Forward: 1545 starts moving
 */
namespace {
    constexpr bool ENABLE_UART_DEBUG {false};
    constexpr uint32_t PWM_TEST_DELAY_MS {2000};
    constexpr uint32_t RECV_PRINTER_STACK_SIZE_BYTES {512};

    constexpr uint16_t ESC_TEST_PULSE_US {1575};

    StaticTask_t recvPrinterTaskCb_;
    StackType_t recvPrinterTaskStack_[RECV_PRINTER_STACK_SIZE_BYTES];

    constexpr osThreadAttr_t recvPrinterTaskAttr_ = {
        .name       = "recvPrintTask",
        .attr_bits  = 0,
        .cb_mem     = &recvPrinterTaskCb_,
        .cb_size    = sizeof(recvPrinterTaskCb_),
        .stack_mem  = recvPrinterTaskStack_,
        .stack_size = sizeof(recvPrinterTaskStack_),
        .priority   = osPriorityNormal,
        .tz_module  = 0,
        .reserved   = 0,
    };


    void recvPrintThread(void *argument)
    {
        (void)argument;

        while (true) {
            uart::DataPacket_raw packet {};
            if (!uart::recv::dequeue(&packet)) {
                continue;
            }

            if (packet.id == uart::ePacketID::HOST_ACK && packet.length == 0) {
                std::printf("RX ack\n\r");
            } else if (packet.id == uart::ePacketID::HOST_DEBUG) {
                std::printf("RX debug: %.*s\n\r", static_cast<int>(packet.length),
                            reinterpret_cast<const char *>(packet.data));
            } else {
                std::printf("RX packet: id=%u, len=%u\n\r",
                            static_cast<unsigned>(packet.id),
                            static_cast<unsigned>(packet.length));
            }
        }
    }


    void startRecvPrintThread()
    {
        const osThreadId_t taskHandle
            = osThreadNew(recvPrintThread, NULL, &recvPrinterTaskAttr_);
        if (taskHandle == nullptr) {
            std::printf(
                "Failed to create recv print task (osThreadNew returned NULL)\n\r");
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
    hal::Ultrasonic HC(&htim1, TIM_CHANNEL_1, GPIOB, GPIO_PIN_10);
    // Hardware test for ESC at 1550 us and servo to the right
    static hal::ESC esc;
    static hal::Servo servo;
    const bool escOk   = esc.init(&htim3);
    const bool servoOk = servo.init(&htim3);

    if (!escOk) {
        std::printf("Failed to initialize HAL ESC test output (esc=%d)\n\r", escOk);
    } else {
        esc.set_pulse_us(hal::ESC::NEUTRAL_US);
    }

    if (!servoOk) {
        std::printf("Failed to initialize HAL Servo output (servo=%d)\n\r", servoOk);
    } else {
        servo.set_pulse_us(hal::Servo::CENTER_US); // MIN_US = right
    }
    osDelay(PWM_TEST_DELAY_MS);

    if (ENABLE_UART_DEBUG) {
        uart::manager::init(&huart1, uart::manager::eUARTInstance::UART_1);
        uart::manager::start();
    }
    if (ENABLE_UART_DEBUG) {
        startRecvPrintThread();
    }


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
