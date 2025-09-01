add_library(hal)
target_sources(hal
  PRIVATE
    src/mock/motors_mock.cpp
)
target_include_directories(hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)