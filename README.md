# PacerBot
A mini “autonomous pace cart” that stays between lane lines and holds target pace.

## Quick start (host build, no hardware)
```bash
mkdir build && cd build
cmake -DBUILD_HOST=ON -DBUILD_STM32=OFF ..
cmake --build . -j
./pacer_host