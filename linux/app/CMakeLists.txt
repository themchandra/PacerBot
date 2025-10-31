# Builds the app layer

include_directories(include)
file(GLOB MY_SOURCES "src/*.cpp")
add_executable(pacerBot ${MY_SOURCES})

# Make use of libraries
target_link_libraries(pacerBot PRIVATE hal comm_uart)