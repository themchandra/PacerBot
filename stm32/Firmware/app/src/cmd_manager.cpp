/**
 * @file cmd_manager.cpp
 * @brief Handles incoming packets from UART module
 * @author Hayden Mai
 * @date May-14-2026
 */

#include "app/cmd_manager.h"

#include "comm/uart/recv.h"

#include <cstring>

namespace app {

    void CMDManager::start()
    {
        if (taskHandle_ != nullptr) {
            return;
        }

        isTaskRunning_ = true;
        taskHandle_    = osThreadNew(threadTrampoline, this, &task_att_);
        if (taskHandle_ == nullptr) {
            isTaskRunning_ = false;
        }
    }


    void CMDManager::stop()
    {
        isTaskRunning_ = false;

        if (taskHandle_ != nullptr) {
            osThreadTerminate(taskHandle_);
            taskHandle_ = nullptr;
        }

        has_target_speed_ = false;
        has_line_pos_     = false;
    }


    bool CMDManager::get_target_speed(float &target_speed_out)
    {
        if (!has_target_speed_) {
            return false;
        }

        target_speed_out  = target_speed_;
        has_target_speed_ = false;
        return true;
    }


    bool CMDManager::get_line_pos(float &line_pos_out)
    {
        if (!has_line_pos_) {
            return false;
        }

        line_pos_out  = line_pos_;
        has_line_pos_ = false;
        return true;
    }


    void CMDManager::threadTrampoline(void *args)
    {
        CMDManager *pThis = static_cast<CMDManager *>(args);
        pThis->threadLoop();
    }


    void CMDManager::threadLoop()
    {
        uart::DataPacket_raw packet {};

        while (isTaskRunning_) {
            if (!uart::recv::dequeue(&packet, osWaitForever)) {
                continue;
            }

            processPacket(packet);
        }
    }


    void CMDManager::processPacket(const uart::DataPacket_raw &packet)
    {
        if (packet.length != sizeof(float)) {
            return;
        }

        float value {};
        std::memcpy(&value, packet.data, sizeof(float));

        if (packet.id == uart::ePacketID::CMD_TARGET_SPEED) {
            target_speed_     = value;
            has_target_speed_ = true;
            return;
        }

        if (packet.id == uart::ePacketID::TELEM_LINE_POS) {
            line_pos_     = value;
            has_line_pos_ = true;
            // Toggle on-board LED to visibly confirm packet reception
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            printf("STM32 got line pos: %f\n\r", value);
        }
    }

} // namespace app
