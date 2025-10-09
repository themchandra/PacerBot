#pragma once

// High-level application logic
namespace app {

    enum class Mode {
        IDLE,  // Normal stop (motors off, safe to resume)
        RUN,   // Driving at commanded speed
        E_STOP // Emergency stop (requires reset)
    };

    // Set target speed in meters per second
    void set_target_speed(float mps);

    // Advance the state machine one cycle
    // dt = elapsed time since last tick (seconds)
    void tick(float dt);

    // Immediately enter E_STOP mode
    void emergency_stop();

    // Get the current operating mode
    Mode mode();

    //  Reset from E_STOP back to IDLE safely
    void reset();

} // namespace app