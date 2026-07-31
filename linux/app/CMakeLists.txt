# Builds the app layer

file(GLOB MY_SOURCES "src/*.cpp")
add_executable(pacerBot ${MY_SOURCES})

target_include_directories(pacerBot PRIVATE
    include
    ../comm/uart/include
)

# Make use of libraries
target_link_libraries(pacerBot PRIVATE hal)

if(TARGET comm_uart)
    target_link_libraries(pacerBot PRIVATE comm_uart)
    target_compile_definitions(pacerBot PRIVATE PACERBOT_HAS_UART)
endif()
