#ifndef ULTRASONIC_H_
#define ULTRASONIC_H_

#include "stm32f4xx_hal.h"
#include <cstdint>

class Ultrasonic {
  public:
    Ultrasonic(TIM_HandleTypeDef *handle, uint32_t channel, GPIO_TypeDef *trig_port,
               uint16_t trig_pin);

    void delay(uint16_t time);
    // Generates a 10 µs trigger pulse on the TRIG pin to start one ultrasonic
    // measurement.
    void trigger();
    void handle_capture_callback();
    float get_distance_cm() const;

    static Ultrasonic *instance;

  private:
    TIM_HandleTypeDef *htim_;
    uint32_t channel_;
    GPIO_TypeDef *trig_port_;
    uint16_t trig_pin_;
    uint32_t ic_val1_    = 0;
    uint32_t ic_val2_    = 0;
    uint32_t diff_       = 0;
    bool first_captured_ = false; // true after a rising edge captured
    float distance_cm_   = 0.0f;

    // constants
    static constexpr float SPEED_OF_SOUND = 0.034f;
};
#endif
