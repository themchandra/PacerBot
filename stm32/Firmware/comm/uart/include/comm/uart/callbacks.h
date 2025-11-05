/**
 * @file callbacks.h
 * @brief Handle UART Callbacks & Interrupts
 * @author Hayden Mai
 * @date Nov-04-2025
 */

#ifndef UART_CALLBACKS_H_
#define UART_CALLBACKS_H_

#include "stm32f4xx_hal.h"
#include <cstdint>

namespace uart::callbacks {
	enum class eUARTPort : uint8_t {
		UART_1,
		UART_2,
	};

    void set_huart(eUARTPort uartPort, UART_HandleTypeDef *huart);

} // namespace uart::callbacks

#endif