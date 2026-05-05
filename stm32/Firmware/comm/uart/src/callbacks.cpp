/**
 * @file callbacks.cpp
 * @brief Handle UART Callbacks & Interrupts
 * @author Hayden Mai
 * @date May-03-2026
 */

#include "comm/uart/callbacks.h"
#include "comm/uart/recv.h"
#include "comm/uart/send.h"

namespace {
    // TODO: If this does grow, put into an array
    UART_HandleTypeDef *huart1_;
    UART_HandleTypeDef *huart2_;
} // namespace


extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    // Forward TX complete to send module for any configured UART handle
    if (huart == huart1_) {
        uart::send::handleTxComplete(huart);
    }
}


extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (size) {
    }

    if (huart == huart1_) {
        uart::recv::updateBufInd(size);
    }
}


namespace uart::callbacks {
    void set_huart(UART_HandleTypeDef *huart, manager::eUARTInstance instance)
    {
        switch (instance) {
        case manager::eUARTInstance::UART_1:
            huart1_ = huart;
            break;

        case manager::eUARTInstance::UART_2:
            huart2_ = huart;
            break;

        default:
            break;
        }
    }
} // namespace uart::callbacks