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
     */
    ~SerialUART();

    /**
     * @brief Opens and configures the UART port. This must be done at least once for
     * 		  read/write operations.
     * @return true if successfully opened & configured, false otherwise.
     */
    bool openPort();


    /**
     * @brief Closes the UART port if it is open.
     */
    void closePort();

    /**
     * @brief Reads data from the UART port.
     * @param buffer Pointer to a character array to store the read data.
     * @param size Maximum number of bytes to read.
     * @return Number of bytes read, or -1 on error.
     */
    int readData(char *buffer, size_t size);

    /**
     * @brief Writes data to the UART port.
     * @param data Pointer to the data to write.
     * @param size Number of bytes to write.
     * @return Number of bytes written, or -1 on error.
     */
    int writeData(const char *data, size_t size);

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
     * @return true if configuration is successful, false otherwise.
     */
    bool configurePort();
};

#endif