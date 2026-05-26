# Builds the app layer

include_directories(include)
file(GLOB MY_SOURCES "src/*.cpp")
add_executable(pacerBot ${MY_SOURCES})

# Make use of libraries
find_package(Threads REQUIRED)
target_link_libraries(pacerBot PRIVATE hal comm_uart comm_ipc Threads::Threads)