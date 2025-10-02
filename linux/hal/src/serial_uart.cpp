/**
 * @file serial_uart.cpp
 * @brief I/O for serial communication (UART) port.
 * @author Hayden Mai
 * @date Oct-02-2025
 *
 * @link
 * https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
 */

#include "serial_uart.h"
#include "exception/serial_exception.h"

#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>


SerialUART::SerialUART(const std::string &device, int baudrate)
    : device_(device), baudrate_(baudrate)
{}


SerialUART::~SerialUART()
{
	closePort();
}


void SerialUART::openPort()
{
    // Open the serial port even if DCD (RS-232 serial) is low or absent
    fd_ = open(device_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1) {
        throw SerialException("Failed to open UART port " + device_ + ": "
                              + strerror(errno));
    }

    // Set to blocking I/O, use (fd, F_SETFL, FNDELAY) for non-blocking
    if (fcntl(fd_, F_SETFL, 0) == -1) {
        throw SerialException("Failed to set UART blocking mode: "
                              + std::string(strerror(errno)));
    }

    configurePort();
    isOpen_ = true;
}


void SerialUART::closePort()
{
    if (isOpen_) {
        if (close(fd_) == -1) {
            throw SerialException("Failed to close UART port: "
                                  + std::string(strerror(errno)));
        }

        isOpen_ = false;
    }
}


ssize_t SerialUART::readData(char *buffer, size_t size)
{
    if (!isOpen_) {
        throw SerialException("Attempted to read, UART port is not open");
    }

    ssize_t bytesRead = read(fd_, buffer, size);
    if (bytesRead < 0) {
        throw SerialException("Failed to read from UART: "
                              + std::string(strerror(errno)));
    }
    return bytesRead;
}


ssize_t SerialUART::writeData(const char *data, size_t size)
{
    if (!isOpen_) {
        throw SerialException("Attempted to write, UART port is not open");
    }

    int bytesWritten = write(fd_, data, size);
    if (bytesWritten < 0) {
        throw SerialException("Failed to write to UART: " + std::string(strerror(errno)));
    }

    return bytesWritten;
}


bool SerialUART::isOpen() const { return isOpen_; }


void SerialUART::configurePort()
{
    // termios stores the general terminal interface to control asynchronous comm ports
    struct termios options;

    if (tcgetattr(fd_, &options) != 0) {
        throw SerialException("Failed to get UART attributes: "
                              + std::string(strerror(errno)));
    }

    // Set serial I/O baud rate
    cfsetispeed(&options, baudrate_);
    cfsetospeed(&options, baudrate_);

    // Configure UART
    // "man 3 termios" in terminal for more information
    options.c_cflag |= (CREAD | CLOCAL); // Enable receiver, ignore modem control lines
    options.c_cflag &= ~CSIZE;           // Clear current character size bits
    options.c_cflag |= CS8;              // 8 data bits per byte
    options.c_cflag &= ~PARENB;          // No parity bit
    options.c_cflag &= ~CSTOPB;          // 1 stop bit
    options.c_cflag &= ~CRTSCTS;         // No hardware flow control (RTS/CTS lines)

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Enable raw mode input
    options.c_iflag &= ~(IXON | IXOFF | IXANY);         // No software flow control
    options.c_oflag &= ~OPOST;                          // Raw outputs

    tcflush(fd_, TCIFLUSH);
    if (tcsetattr(fd_, TCSANOW, &options) != 0) { // Apply settings immediately
        throw SerialException("Failed to set UART attributes: "
                              + std::string(strerror(errno)));
    }
}