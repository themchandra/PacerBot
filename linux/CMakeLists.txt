cmake_minimum_required(VERSION 3.18)
project(PacerBot 
		VERSION 0.1
		LANGUAGES CXX)

# Compiler options

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_options(-Wall -Werror -Wpedantic -Wextra)

# Folders to build
add_subdirectory(hal)
add_subdirectory(comm)
add_subdirectory(app)