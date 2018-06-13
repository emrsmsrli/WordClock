cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)
set(PROJECT_NAME wordclock)
project(${PROJECT_NAME})

set(${CMAKE_PROJECT_NAME}_SKETCH wordclock.ino)

generate_arduino_firmware(${CMAKE_PROJECT_NAME}
        SRCS wordclock.ino Button.cpp LedArray.cpp Music.cpp
        HDRS wordclock.hpp Button.hpp LedArray.hpp Music.h
        BOARD uno
        PORT COM7
        PROGRAMMER avrisp)
