/*
 * uart_comms.h
 *
 *  Created on: Sep 25, 2025
 *      Author: Hayden Mai
 */

#ifndef UART_COMMS_H_
#define UART_COMMS_H_

#include "FreeRTOS.h"
#include "queue.h"

class UART_Comms {
public:
	// Configuration
	static const uint8_t MAX_MSG_SIZE {128};

	// Message structure
	struct Message {
		uint8_t data[MAX_MSG_SIZE];
		uint8_t len;
	};


	UART_Comms(UART_HandleTypeDef* huart,
               size_t queueLength_tx = 10,
			   size_t queueLength_rx = 10);
	~UART_Comms();


	// --- Transmit Messages ---
	bool enqueue_send(const uint8_t *data, size_t length);
	void dequeue_send(struct Message &msg);
	void peek_send(struct Message &msg);
	bool isQueueEmpty_send(void) const;
	uint8_t size_send(void) const;
	bool clearQueue_send(void);

	// Dequeue & transmit the message
	bool process_send();


	// --- Receiving Messages ---
	bool enqueue_recv(const uint8_t *data, size_t length);
	void dequeue_recv(struct Message &msg);
	void peek_recv(struct Message &msg);
	bool isQueueEmpty_recv(void) const;
	uint8_t size_recv(void) const;
	bool clearQueue_recv(void);

private:
	UART_HandleTypeDef* huart_;

	QueueHandle_t queue_rx_;
	QueueHandle_t queue_tx_;
};


#endif /* UART_COMMS_H_ */
