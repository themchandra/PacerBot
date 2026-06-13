/**
 * @file callbacks.h
 * @brief Handle UART Callbacks & Interrupts
 * @author Hayden Mai
 * @date Dec-13-2025
 */

#ifndef UART_CALLBACKS_H_
#define UART_CALLBACKS_H_

#include "comm/uart/manager.h"
#include "hal/gps.h"

namespace uart::callbacks {
    /**
     * @brief Assign UART ports for callbacks to compare.
     * @param huart Pointer of UART initialized by main.c
     * @param instance Enum corresponding to the UART port
     */
    void set_huart(UART_HandleTypeDef *huart, manager::eUARTInstance instance);

    /**
     * @brief Register the GPS instance to receive RX events.
     * @param gps Pointer to the GPS instance whose UART is on USART6.
     */
    void set_gps(hal::GPS *gps);

} // namespace uart::callbacks

#endif