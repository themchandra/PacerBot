/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date May-08-2026
 */

#include "app/app_main.h"
#include "comm/uart/manager.h"
#include "comm/uart/recv.h"

#include "cmsis_os.h"
#include "hal/esc.h"
#include "hal/imu.h"
#include "hal/servo.h"
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

// External global variables from Core/Src/main.c
// NOTE: Only pass them as reference through initialization for modules/classes,
// 		 save them as a pointer for use.
extern "C" {
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern TIM_HandleTypeDef htim3;
}


extern "C" void app_main(void)
{
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
        servo.set_pulse_us(hal::Servo::MIN_US); // Keep servo to the right

        esc.set_pulse_us(1425); // Backward
        osDelay(PWM_TEST_DELAY_MS);
        esc.set_pulse_us(1575); // Forward 

        if (ENABLE_UART_DEBUG) {
            static uint32_t debugCounter {1};
            char debugMessage[32] {};
            std::snprintf(debugMessage, sizeof(debugMessage), "Debug here! #%lu",
                          static_cast<unsigned long>(debugCounter++));
            uart::send::enqueue_debug(debugMessage);
            uart::send::enqueue_ack();
        }

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        osDelay(PWM_TEST_DELAY_MS);
    }
}