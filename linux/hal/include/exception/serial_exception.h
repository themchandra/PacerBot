/**
 * @file serial_uart.h
 * @brief I/O for serial communication (UART) port.
 * @author Hayden Mai
 * @date Oct-02-2025
 */

#ifndef SERIAL_EXCEPTION_H_
#define SERIAL_EXCEPTION_H_

#include <stdexcept>
#include <string>

class SerialException : public std::runtime_error {
  public:
    explicit SerialException(const std::string &message) : std::runtime_error(message) {}
};

#endif