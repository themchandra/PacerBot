#include "comm/uart/manager.h"
#include "comm/uart/send.h"

#include "cmsis_os.h"
#include "main.h"

extern "C" {
extern UART_HandleTypeDef huart1;
}

extern "C" void app_main(void)
{
    uart::manager::init(&huart1, uart::manager::eUARTInstance::UART_1);
    uart::manager::start();

    osDelay(500);

    uart::send::enqueue_ack();

    while (true) {
        osDelay(1000);
    }
}
