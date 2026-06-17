/**
 * @file cmd_manager.h
 * @brief Handles incoming packets from UART module
 * @author Hayden Mai
 * @date Jun-15-2026
 */

#ifndef APP_CMD_MANAGER_H_
#define APP_CMD_MANAGER_H_

#include "comm/uart/packet_info.h"

#include "cmsis_os.h"

#include <cstdint>

namespace app {

    class ControlLoop;

    /**
     * @class CMDManager
     * @brief Consumes UART command packets and forwards them to ControlLoop.
     *
     * The task dequeues validated packets from `uart::recv` and dispatches float
     * payload values to the corresponding ControlLoop PID inputs.
     */
    class CMDManager {
      public:
        /**
         * @brief Construct command manager bound to a control loop instance.
         * @param control_loop Control loop that receives parsed command values.
         */
        explicit CMDManager(ControlLoop &control_loop);

        /**
         * @brief Start the command-processing task.
         * @note If already running, this call does nothing.
         */
        void start();

        /**
         * @brief Stop command-processing task and clear unread command flags.
         */
        void stop();

      private:
        static constexpr std::uint32_t STACK_SIZE_BYTES {512};
        static constexpr osThreadAttr_t task_att_ {
            .name       = "CMDManagerTask",
            .attr_bits  = 0,
            .cb_mem     = nullptr,
            .cb_size    = 0,
            .stack_mem  = nullptr,
            .stack_size = STACK_SIZE_BYTES,
            .priority   = osPriorityNormal,
            .tz_module  = 0,
            .reserved   = 0,
        };

        ControlLoop &control_loop_;

        osThreadId_t taskHandle_ {};
        volatile bool isTaskRunning_ {false};

        /**
         * @brief Static RTOS entrypoint that forwards to `threadLoop()`.
         * @param args Pointer to CMDManager instance.
         */
        static void threadTrampoline(void *args);

        /**
         * @brief Main loop that dequeues and processes command packets.
         */
        void threadLoop();

        /**
         * @brief Decode and route a received packet into command state.
         * @param packet Validated UART packet from recv queue.
         */
        void processPacket(const uart::DataPacket_raw &packet);
    };

} // namespace app

#endif
