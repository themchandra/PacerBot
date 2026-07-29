/**
 * @file app_main.cpp
 * @brief Initialize modules needed. Should be called from Core/main.c
 * @author Hayden Mai
 * @date Jul-29-2026
 */

#include "app/app_main.h"
#include "app/cmd_manager.h"
#include "app/control_loop.h"

#include "comm/uart/callbacks.h"
#include "comm/uart/manager.h"
#include "comm/uart/recv.h"

#include "cmsis_os.h"
#include "main.h"

#include <cstdio>

// External global variables from Core/Src/main.c
// NOTE: Only pass them as reference through initialization for modules/classes,
// 		 save them as a pointer for use.
//       DO NOT use 'extern' anywhere else outside of this file!!
extern "C" {
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern TIM_HandleTypeDef htim3;
}

extern "C" void app_main(void)
{
    uart::manager::init(&huart1, uart::manager::eUARTInstance::UART_1);
    uart::manager::start();

    uart::DataPacket_raw packet {};

    while (true) {
        if (uart::recv::dequeue(&packet, osWaitForever)) {
            if (packet.id == uart::ePacketID::HOST_DEBUG) {
                printf("Received from Linux: ");

                for (uint8_t i = 0; i < packet.length; ++i) {
                    printf("%c", packet.data[i]);
                }

                printf("\r\n");
            }
        }
    }
}
