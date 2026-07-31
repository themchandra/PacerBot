#ifndef MANUAL_CONTROL_H_
#define MANUAL_CONTROL_H_

#include <cstdint>

struct ManualControl {
    bool isForward {false};
    bool isReverse {false};
    bool isLeft {false};
    bool isRight {false};
};

/**
 * @brief Polls keyboard input and updates the latched manual control state.
 *
 * Controls:
 * - w/s: set forward/reverse
 * - x: clear forward/reverse
 * - a/d: set left/right
 * - q: clear left/right
 * - space: clear all motion
 *
 * If no key is available, the previous control state is preserved.
 */
void updateManualControl(ManualControl &control);

/**
 * @brief Encodes manual control state into the CMD_MCTL UART payload byte.
 *
 * Bit layout is defined by uart::CMD_MCTL_* masks in packet_info.h.
 */
uint8_t encodeManualControl(const ManualControl &control);

#endif