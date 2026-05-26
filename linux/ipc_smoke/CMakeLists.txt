cmake_minimum_required(VERSION 3.18)

project(ipc_smoke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Minimal reader-only smoke test for the lane FIFO.
add_executable(ipc_smoke
	src/main.cpp
	../comm/ipc/src/lane_pipe_reader.cpp
)

target_include_directories(ipc_smoke PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/../comm/ipc/include
)
