cmake_minimum_required(VERSION 3.11)
project(espasyncwebserver_host_compile LANGUAGES C CXX)

add_subdirectory(${CMAKE_SOURCE_DIR}/../../.ci/arduino-emulator ${CMAKE_BINARY_DIR}/arduino-emulator)
file(GLOB WEB_SRC "${CMAKE_SOURCE_DIR}/../../src/*.cpp")
add_library(espasyncwebserver STATIC ${WEB_SRC})
add_library(test STATIC
  ${CMAKE_SOURCE_DIR}/../../.ci/asynctcp/src/AsyncTCP.cpp
  ${CMAKE_SOURCE_DIR}/../../.ci/arduino-esp32/libraries/FS/src/FS.cpp
  ${CMAKE_SOURCE_DIR}/../../.ci/arduino-emulator/ArduinoCore-Linux/cores/arduino/libb64/cencode.c
)

target_compile_definitions(espasyncwebserver PUBLIC HOST ARDUINO=10813)
target_include_directories(espasyncwebserver PUBLIC
  ${CMAKE_SOURCE_DIR}/../../src
  ${CMAKE_SOURCE_DIR}/../../.ci/asynctcp/src
  ${CMAKE_SOURCE_DIR}/../../.ci/arduino-esp32/libraries/FS/src
)
target_link_libraries(espasyncwebserver PUBLIC arduino_emulator)

target_compile_definitions(test PUBLIC HOST ARDUINO=10813)
target_include_directories(test PUBLIC
  ${CMAKE_SOURCE_DIR}/../../.ci/asynctcp/src
  ${CMAKE_SOURCE_DIR}/../../.ci/arduino-esp32/libraries/FS/src
)
target_link_libraries(test PUBLIC arduino_emulator)

add_executable(espasyncwebserver_host main.cpp)
target_link_libraries(espasyncwebserver_host PRIVATE espasyncwebserver test)
