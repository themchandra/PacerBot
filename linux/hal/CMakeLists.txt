# CMakeList.txt for HAL
#   Build a library (`hal`) which exposes the header files as "hal/*.h"
#   Use header as: #include "hal/button.h"

include_directories(hal/include)
file(GLOB MY_SOURCES "src/*.cpp")		 # Collect .cpp files from src
file(GLOB MOCK_SOURCES "src/mock/*.cpp") # Collect .cpp files from src/mock

add_library(hal STATIC ${MY_SOURCES} ${MOCK_SOURCES})
target_include_directories(hal PUBLIC include)