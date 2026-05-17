# CMakeLists.txt for comm/ipc

# Build IPC communication library

file(GLOB MY_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(comm_ipc STATIC ${MY_SOURCES})

# Expose headers

target_include_directories(comm_ipc PUBLIC include)