# CMakeList.txt for comm/uart
#   Build a library (`comm/uart`) which exposes the header files as "comm/uart/*.h"
#   Use header as: #include "comm/uart/button.h"

include_directories(uart/include)
file(GLOB MY_SOURCES "src/*.cpp")
add_library(comm_uart STATIC ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(comm_uart PRIVATE hal)

# Expose its local include directory for "comm/uart/uart_comm.h"
target_include_directories(comm_uart PUBLIC include)