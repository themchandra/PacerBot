/**
 * @file SerialUART.cpp
 * @brief I/O for serial communication (UART) port.
 * @author Hayden Mai
 * @date May-01-2026
 *
 * @link
 * https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
 */

#include "hal/SerialUART.h"
#include "hal/exception/SerialException.h"

#include <asm/termbits.h>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>


SerialUART::SerialUART(const std::string &device, int baudrate, int timeout_sec)
    : device_(device), baudrate_(baudrate), timeout_sec_(timeout_sec)
{}


SerialUART::~SerialUART() { closePort(); }


void SerialUART::openPort()
{
    // Open serial port in blocking mode.
    // O_RDWR: Read and write access
    // O_NOCTTY: Don't make this the controlling terminal for the process
    fd_ = open(device_.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ == -1) {
        throw SerialException("Failed to open UART port " + device_ + ": "
                              + strerror(errno));
    }

    configurePort();

    if (ioctl(fd_, TCFLSH, TCIOFLUSH) != 0) {
        throw SerialException("Failed to flush UART buffers: "
                              + std::string(strerror(errno)));
    }

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


ssize_t SerialUART::readData(uint8_t *buffer, size_t size) const
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


ssize_t SerialUART::writeData(const uint8_t *data, size_t size) const
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


void SerialUART::setTimeout(int seconds)
{
    timeout_sec_ = seconds;
    configurePort();
}


void SerialUART::configurePort() const
{
    struct termios2 tio;

    // Read current UART settings and update only required fields.
    if (ioctl(fd_, TCGETS2, &tio) != 0) {
        throw SerialException("Failed to get UART attributes: "
                              + std::string(strerror(errno)));
    }

    // Match uart-test setup: custom arbitrary baud via BOTHER.
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = baudrate_;
    tio.c_ospeed = baudrate_;

    tio.c_cflag &= ~PARENB;
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cflag |= (CLOCAL | CREAD);

    // Timed blocking reads: prevents tight spin when no data is available.
    // VMIN=0 and VTIME>0 means read returns on first byte or timeout.
    const int timeout_ds = (timeout_sec_ > 0) ? (timeout_sec_ * 10) : 1;
    tio.c_cc[VMIN]       = 0;
    tio.c_cc[VTIME]      = timeout_ds;

    if (ioctl(fd_, TCSETS2, &tio) != 0) {
        throw SerialException("Failed to set UART attributes: "
                              + std::string(strerror(errno)));
    }
}