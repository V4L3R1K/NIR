cmake_minimum_required(VERSION 3.20)

project(NIR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

set(SOURCES
    src/main.cpp

    src/model/io/ImageLoader.cpp
    src/model/io/ImageSaver.cpp

    src/model/steganography/lsb/LSBEncoder.cpp
)

add_executable(main ${SOURCES})