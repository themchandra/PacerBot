cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME pacerbot_f411re)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME}
		DESCRIPTION "A mini cart that stays between lane lines and holds a target pace. STM32"
		VERSION 0.1
		LANGUAGES C CXX ASM)

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add retarget.c to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE Core/Src/retarget.c)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
	hal
	app
	comm_uart
)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -Wl,-u,_printf_float)
add_compile_options(-Wall -Werror -Wpedantic -Wextra -fno-exceptions -fno-rtti)

# Folders to build
add_subdirectory(Firmware)


# Add a custom command to create .bin from .elf
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Converting ELF → BIN"
)

# Automatically flash after build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND st-flash write ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin 0x08000000
    COMMENT "Flashing ${PROJECT_NAME}.bin to STM32"
)