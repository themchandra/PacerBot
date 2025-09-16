add_library(app_core)
target_sources(app_core PRIVATE src/state_machine.cpp)
target_include_directories(app_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(app_core PUBLIC hal)