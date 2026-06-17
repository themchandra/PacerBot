/**
 * @file cmd_manager.cpp
 * @brief Handles incoming packets from UART module
 * @author Hayden Mai
 * @date Jun-15-2026
 */

#include "app/cmd_manager.h"
#include "app/control_loop.h"

#include "comm/uart/recv.h"

#include <cstring>

namespace app {

    CMDManager::CMDManager(ControlLoop &control_loop) : control_loop_(control_loop) {}


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
            control_loop_.set_target_speed(value);
            return;
        }

        if (packet.id == uart::ePacketID::TELEM_LINE_POS) {
            control_loop_.set_measured_line_position(value);
        }
    }

} // namespace app
