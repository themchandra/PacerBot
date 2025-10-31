# CMakeList.txt for HAL 
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

file(GLOB MY_SOURCES "src/*.cpp")
add_library(hal STATIC ${MY_SOURCES})

# Make use of the STM32 library
target_link_libraries(hal PRIVATE stm32cubemx)

# Expose its local include directory for "app/*.h"
target_include_directories(hal PUBLIC include)
