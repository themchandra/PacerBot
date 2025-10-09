/*
 * uart_comm.h
 *
 *  Created on: Sep 25, 2025
 *      Author: Hayden Mai
 */

#ifndef UART_COMM_H_
#define UART_COMM_H_

#include "FreeRTOS.h"
#include "queue.h"

class UART_Comm {
  public:
    // Configuration
    static const uint8_t MAX_MSG_SIZE {128};

    // Message structure
    struct Message {
        uint8_t data[MAX_MSG_SIZE];
        uint8_t len;
    };


    UART_Comm(UART_HandleTypeDef *huart, size_t queueLength_tx = 10,
              size_t queueLength_rx = 10);
    ~UART_Comm();


    // Transmit Messages
    bool send_enqueue(const uint8_t *data, size_t length);
    void send_dequeue(struct Message &msg);
    void send_peek(struct Message &msg);
    bool send_isQueueEmpty(void) const;
    uint8_t send_queueSize(void) const;
    bool send_clearQueue(void);
    bool send_process(); // Dequeue & transmit the message

    // Receiving Messages
    bool recv_enqueue(const uint8_t *data, size_t length);
    void recv_dequeue(struct Message &msg);
    void recv_peek(struct Message &msg);
    bool recv_isQueueEmpty(void) const;
    uint8_t recv_queueSize(void) const;
    bool recv_clearQueue(void);

  private:
    UART_HandleTypeDef *huart_;

    QueueHandle_t queue_rx_;
    QueueHandle_t queue_tx_;
};


#endif /* UART_COMM_H_ */
