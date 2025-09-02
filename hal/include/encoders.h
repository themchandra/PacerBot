#pragma once

namespace hal::encoders {
    // Get current speed from encoders (m/s)
    float get_speed();
    
    // Reset encoder counters
    void reset();
    
    // Update simulation (only used in mock implementation)
    void update_simulation(float dt);

}
