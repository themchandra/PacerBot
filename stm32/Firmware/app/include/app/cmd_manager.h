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
    /**
     * @class CMDManager
     * @brief Consumes UART command packets and exposes latest command values.
     *
     * The task dequeues validated packets from `uart::recv` and extracts float
     * payload values for target speed and line position commands.
     */
    class CMDManager {
      public:
        /**
         * @brief Construct command manager with cleared command state.
         */
        CMDManager() = default;

        /**
         * @brief Start the command-processing task.
         * @note If already running, this call does nothing.
         */
        void start();

        /**
         * @brief Stop command-processing task and clear unread command flags.
         */
        void stop();

        /**
         * @brief Fetch latest unread target speed command.
         * @param target_speed_out Output speed value.
         * @return true if new value is available, false otherwise.
         */
        bool get_target_speed(float &target_speed_out);

        /**
         * @brief Fetch latest unread lines position telemetry.
         * @param line_pos_out Output current position value.
         * @return true if new value is available, false otherwise.
         */
        bool get_line_pos(float &line_pos_out);

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

        osThreadId_t taskHandle_ {};
        volatile bool isTaskRunning_ {false};

        float target_speed_ {};
        float line_pos_ {};
        volatile bool has_target_speed_ {false};
        volatile bool has_line_pos_ {false};

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
