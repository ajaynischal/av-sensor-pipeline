cmake_minimum_required(VERSION 3.16)
project(av-sensor-pipeline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -O2)

find_package(Threads REQUIRED)

add_executable(av-sensor-pipeline
  src/main.cpp
  src/pipeline.cpp
  src/sensor_producer.cpp
  src/watchdog.cpp
)
target_include_directories(av-sensor-pipeline PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(av-sensor-pipeline PRIVATE Threads::Threads)

add_executable(test_ring_buffer tests/test_ring_buffer.cpp)
target_include_directories(test_ring_buffer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test_ring_buffer PRIVATE Threads::Threads)

add_executable(test_watchdog
  tests/test_watchdog.cpp
  src/watchdog.cpp
  src/sensor_producer.cpp
)
target_include_directories(test_watchdog PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(test_watchdog PRIVATE Threads::Threads)
