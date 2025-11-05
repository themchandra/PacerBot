/**
 * @file callbacks.cpp
 * @brief Handle UART Callbacks & Interrupts
 * @author Hayden Mai
 * @date Nov-04-2025
 */

#include "comm/uart/callbacks.h"
#include "comm/uart/recv.h"


namespace {
    // TODO: If this does grow, put into an array
    UART_HandleTypeDef *huart1_;
    UART_HandleTypeDef *huart2_;
} // namespace


extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (size) {
    }

    if (huart == huart1_) {
    }

    if (huart == huart2_) {
        HAL_UART_Transmit(huart, (uint8_t *)"Leave\r\n", 7, 100);
    }
}


namespace uart::callbacks {
    void set_huart(eUARTPort uartPort, UART_HandleTypeDef *huart)
    {
        switch (uartPort) {
        case eUARTPort::UART_1:
            huart1_ = huart;
            break;

        case eUARTPort::UART_2:
            huart2_ = huart;
            break;

        default:
            break;
        }
    }
} // namespace uart::callbacks