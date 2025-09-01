# PacerBot
A mini “autonomous pace cart” that stays between lane lines and holds a target pace.  
Inspired by stadium pacing lights (like WaveLight), but portable and affordable for everyday runners.

---

## Motivation
Runners often struggle to hold a consistent pace during workouts. Smartwatches such as Apple Watch or Garmin provide feedback only **after** a runner has already drifted off pace, forcing them to glance at their wrist and break rhythm.  

Elite athletes sometimes train with pacing lights, like the **WaveLight system**, which guide them lap by lap. However, these are **expensive, fixed to tracks, and inaccessible** to everyday runners.  

**PacerBot** aims to bring accurate pacing to any runner by providing a lightweight, track-ready autonomous cart.

---

## Key Features
- **Accurate Pacing Control** – maintains target lap times within ±1s per 400m  
- **Lane Following** – stays centered in a lane using line sensors  
- **Safety Systems** – emergency stop switch, bumper cut-off, and speed cap  
- **Wireless Control** – set pace, start/stop, and E-stop from phone/host app via BLE  
- **Telemetry Logging** – logs target vs. measured speed, lap splits, and states to CSV  
- **Portable Design** – <3 kg, 30–40 min runtime on battery  

## Milestones
- [ ] **Milestone 1 – Rolling Base**  
  Assemble chassis, motors, wheels, battery.  
  Encoder ISRs + simple velocity PID (drive straight).  

- [ ] **Milestone 2 – Pace Accuracy**  
  Calibrate wheel circumference & encoders, tune PID.  
  ±1s/400 m accuracy on straight runs.  

- [ ] **Milestone 3 – Lane Following**  
  Add QTR-8 sensor array.  
  Lateral PID keeps cart centered.  

- [ ] **Milestone 4 – Safety Systems**  
  Emergency stop button, bumper switch, SAFE_STOP (<100ms cutoff).  

- [ ] **Milestone 5 – Connectivity & Logging**  
  BLE/UART pace commands, Python logger, basic UI.  

- [ ] **Milestone 6 – Track Demo (MVP)**  
  6 laps at fixed pace with logged splits + demo video.  

---

## Quick Start (Host Build, No Hardware)
```bash
# From project root
mkdir -p build && cd build

# Configure for host build
cmake -G Ninja -DBUILD_HOST=ON -DBUILD_STM32=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Build
cmake --build . -j

# Symlink compile_commands.json (for VS Code / clangd IntelliSense)
ln -s build/compile_commands.json ..

# Run simulator
./pacerbot
