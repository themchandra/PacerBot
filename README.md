# PacerBot

<p align="center">
  <img src="docs/images/pacerbot_img.jpg" alt="PacerBot" width="640" />
</p>

PacerBot is an autonomous RC vehicle designed to help runners maintain a target pace.

The project combines embedded firmware, computer vision, and real-time control to create a portable pacing system for running workouts.

## Motivation

It can be difficult for runners to hold a consistent pace during workouts. 

Running watches display pace information, but runners must still repeatedly check their watch and adjust their speed.

Professional pacing systems such as WaveLight exist, but they are expensive and only available at certain tracks and events.

PacerBot was built as a lower-cost, portable pacing system that can autonomously drive around a track while maintaining a target pace.

## System Architecture

```text
Camera
  ↓
BeagleY-AI
  - Vision processing
  - High-level control
  ↓ UART
STM32
  - Real-time motor control
  - PID controllers
  - Sensor integration
  - Safety monitoring
  ↓ PWM
ESC + Servo + Drive Motor
```

## Repository Structure

```text
linux/
  Linux host application

stm32/    
  STM32 firmware
```

## Hardware Used

### Core Components

- BeagleY-AI SBC
- STM32F411RE microcontroller
- IMX219 CSI camera
- 1/10 scale RC car chassis
- RC battery
- Brushed ESC
- Steering servo
- Drive motor
- IMU 
- GPS receiver
- Ultrasonic distance sensor

## Development Tools
- CMake
- Ninja
- GCC / Clang
- arm-none-eabi toolchain
- STM32CubeMX
- OpenCV
- GStreamer
- Git

## Current Status

- ✅ Bidirectional UART communication between BeagleY-AI and STM32
- ✅ ESC and steering control operational
- ✅ Manual control over UART
- ✅ GPS, IMU, and ultrasonic sensor integration
- 🔄 Vision pipeline under development
- 🔄 Autonomous lane following in progress

## UART Communication

The Beagle and STM32 communicate over UART using lightweight command packets for:

- start/stop commands
- throttle/speed control
- steering commands
- telemetry and status messages

