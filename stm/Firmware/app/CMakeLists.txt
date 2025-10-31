# CMakeList.txt for app
#   Build a library (`app`) which exposes the header files as "app/*.h"
#   Use header as: #include "app/button.h"

file(GLOB MY_SOURCES "src/*.cpp")
add_library(app STATIC ${MY_SOURCES})

# Make use of the STM32 library
target_link_libraries(app PRIVATE stm32cubemx)

# Expose its local include directory for "app/*.h"
target_include_directories(app PUBLIC include)
