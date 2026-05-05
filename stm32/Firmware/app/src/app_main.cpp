/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date May-05-2026
 */

#include "app/app_main.h"
#include "comm/uart/manager.h"
#include "comm/uart/recv.h"

#include "cmsis_os.h"
#include "hal/imu.h"
#include "main.h"

#include <cstdio>

namespace {
    constexpr uint32_t RECV_PRINTER_STACK_SIZE_BYTES {512};

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
    // initialize other modules and start new threads and stuff
    // call C++ functions here
    uart::manager::init(&huart1, uart::manager::eUARTInstance::UART_1);
    uart::manager::start();
    startRecvPrintThread();
    uint32_t debugCounter {1};
    // std::array<float, 3> a_data;
    // IMU MPU6050(&hi2c1, 0x68);
    // MPU6050.scan_i2c();

    while (true) {
        char debugMessage[32] {};
        std::snprintf(debugMessage, sizeof(debugMessage), "Debug here! #%lu",
                      static_cast<unsigned long>(debugCounter++));
        uart::send::enqueue_debug(debugMessage);
        osDelay(100);
        uart::send::enqueue_ack();

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        // MPU6050.get_accel(a_data);
        // printf("x =%f, y=%f , z=%f\n", a_data[0], a_data[1], a_data[2]);
        osDelay(100);
    }
}