/**
 * @file SerialUART.cpp
 * @brief I/O for serial communication (UART) port.
 * @author Hayden Mai
 * @date Oct-30-2025
 *
 * @link
 * https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
 */

#include "hal/SerialUART.h"
#include "hal/exception/SerialException.h"

#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>


SerialUART::SerialUART(const std::string &device, int baudrate, int timeout_sec)
    : device_(device), baudrate_(baudrate), timeout_sec_(timeout_sec)
{}


SerialUART::~SerialUART() { closePort(); }


void SerialUART::openPort()
{
    // Open serial port in blocking read/write mode
    // O_RDWR: Read and write access
    // O_NOCTTY: Don't make this the controlling terminal for the process
    fd_ = open(device_.c_str(), O_RDWR | O_NOCTTY);
    if (fd_ == -1) {
        throw SerialException("Failed to open UART port " + device_ + ": "
                              + strerror(errno));
    }

    // Control DTR (Data Terminal Ready) and RTS (Request To Send) modem lines
    // Many USB-serial adapters connect these lines to the microcontroller's reset pin
    // Clearing them prevents the STM32 from resetting when we open the port
    int status;
    ioctl(fd_, TIOCMGET, &status); // Get current modem line status
    status &= ~TIOCM_DTR;          // Clear DTR line
    status &= ~TIOCM_RTS;          // Clear RTS line
    ioctl(fd_, TIOCMSET, &status); // Apply the changes

    // Wait for hardware to stabilize after modem line changes
    usleep(100000);
    configurePort();

    // Drain any stale/garbage data that may have accumulated in the buffer
    // This clears data that was sent before our program was ready to receive
    tcflush(fd_, TCIOFLUSH); // Flush both input and output buffers
    usleep(200000);
    tcflush(fd_, TCIOFLUSH); // Flush again to catch any late arrivals

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
    struct termios options;
    memset(&options, 0, sizeof(options)); // Zero out structure

    constexpr int MIN_BYTES {1};
    const int TIMEOUT_DS {timeout_sec_ * 10}; // Timeout in deciseconds

    options.c_cflag = B115200 | CS8 | CREAD | CLOCAL; // 115200 baud, 8N1, raw mode
    options.c_cflag &= ~PARENB;                       // No parity
    options.c_cflag &= ~CSTOPB;                       // 1 stop bit
    options.c_cflag &= ~CRTSCTS;                      // No hardware flow control

    options.c_iflag = IGNPAR; // Ignore parity errors
    options.c_oflag = 0;      // Raw output
    options.c_lflag = 0;      // Raw input

    options.c_cc[VMIN]  = MIN_BYTES;  // Block until at least 1 byte
    options.c_cc[VTIME] = TIMEOUT_DS; // 1 second timeout

    tcflush(fd_, TCIOFLUSH); // Flush buffers before applying settings

    if (tcsetattr(fd_, TCSANOW, &options) != 0) {
        throw SerialException("Failed to set UART attributes: "
                              + std::string(strerror(errno)));
    }

    tcdrain(fd_); // Wait for settings to apply
}