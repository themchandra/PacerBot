/**
 * @file callbacks.cpp
 * @brief Handle UART Callbacks & Interrupts
 * @author Hayden Mai
 * @date Jun-13-2026
 */

#include "comm/uart/callbacks.h"
#include "comm/uart/recv.h"
#include "comm/uart/send.h"
#include "hal/gps.h"

namespace {
    // TODO: If this does grow, put into an array
    UART_HandleTypeDef *huart1_ {nullptr};
    UART_HandleTypeDef *huart2_ {nullptr};
    hal::GPS *gps_ {nullptr};
} // namespace


extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == huart1_) {
        uart::send::handleTxComplete(huart);
    }
}


extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    if (huart == huart1_) {
        uart::recv::updateBufInd(size);
    } else if (gps_ != nullptr && huart == gps_->getHuart()) {
        gps_->onRxEvent(size);
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


    void set_gps(hal::GPS *gps) { gps_ = gps; }
} // namespace uart::callbacks