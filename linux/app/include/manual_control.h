#ifndef MANUAL_CONTROL_H_
#define MANUAL_CONTROL_H_

#include <cstdint>

struct ManualControl {
    bool isForward {false};
    bool isReverse {false};
    bool isLeft {false};
    bool isRight {false};
};

void updateManualControl(ManualControl &control);
uint8_t encodeManualControl(const ManualControl &control);

#endif