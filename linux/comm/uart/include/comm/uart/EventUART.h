/**
 * @file EventUART.h
 * @brief Notify and waits for when there is a new message
 * @author Hayden Mai
 * @date Nov-07-2025
 */

#ifndef COMM_UART_EVENTUART_H_
#define COMM_UART_EVENTUART_H_

#include <condition_variable>
#include <mutex>

namespace uart {
    enum class eEvent {
        NONE,
        TELEMETRY,
        STATUS,
        ACK,
        DEBUG,
    };


    class EventFlag {
      public:
        void subscribe();
        void unsubscribe();
        void notify(eEvent event);
        eEvent wait();
        void reset();

      private:
        eEvent event_ {eEvent::NONE};
        std::mutex mtx_ {};
        std::condition_variable cv_ {};

        int num_subscribers_ {};
        int ack_count_ {};
    };

} // namespace uart


#endif