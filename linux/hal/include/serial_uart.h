/**
 * @file serial_uart.h
 * @brief I/O for serial communication (UART) port.
 * @author Hayden Mai
 * @date Oct-02-2025
 *
 * @link
 * https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/
 */

#ifndef SERIAL_UART_H_
#define SERIAL_UART_H_

#include <iostream>

/**
 * @class SerialUART
 * @brief A class for serial communication using UART.
 *
 * This class provides UART functionality including opening, closing,
 * reading from, and writing to a serial port. All error conditions now throw
 * exceptions of type SerialException.
 */
class SerialUART {
  public:
    /**
     * @brief Constructs a SerialUART object.
     * @param device Path to the UART device file (e.g. "/dev/ttyS0").
     * @param baudrate Baud rate for the serial communication (e.g. B9600, B115200).
     */
    SerialUART(const std::string &device, int baudrate);

    /**
     * @brief Closes the port if open and destruct the object instance.
     * @throws SerialException if closing the port fails.
     */
    ~SerialUART();

    /**
     * @brief Opens and configures the UART port. Must be called before any read/write
     * operations.
     * @throws SerialException if port cannot be opened or configured.
     */
    void openPort();


    /**
     * @brief Closes the UART port if it is open.
     * @throws SerialException if closing the port fails.
     */
    void closePort();

    /**
     * @brief Reads data from the UART port.
     * @param buffer Pointer to a character array to store the read data.
     * @param size Maximum number of bytes to read.
     * @return Number of bytes read.
     * @throws SerialException if closing the port fails.
     */
    ssize_t readData(char *buffer, size_t size);

    /**
     * @brief Writes data to the UART port.
     * @param data Pointer to the data to write.
     * @param size Number of bytes to write.
     * @return Number of bytes written.
     * @throws SerialException if the port is not open or a write error occurs.
     */
    ssize_t writeData(const char *data, size_t size);

    /**
     * @brief Checks if the UART port is currently open.
     * @return true if the port is open, false otherwise.
     */
    bool isOpen() const;

  private:
    std::string device_;  ///< Path to UART device file.
    int baudrate_;        ///< Baud rate for communication.
    int fd_ {-1};         ///< File Descriptor for the UART port.
    bool isOpen_ {false}; ///< Indicates if UART port is currently open.

    /**
     * @brief Configures the UART port with the specified settings.
     * @throws SerialException if configuration fails.
     */
    void configurePort();
};

#endif