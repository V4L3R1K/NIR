cmake_minimum_required(VERSION 3.20)

project(NIR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -O3
set(CMAKE_CXX_FLAGS "-O3 -march=native")

# OpenMP
find_package(OpenMP REQUIRED)

# libjpeg
find_package(JPEG REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${JPEG_INCLUDE_DIR}
)

# libcorrect (Reed-Solomon)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/libcorrect)
include_directories(${CMAKE_SOURCE_DIR}/third_party/libcorrect/include)

set(SOURCES
    src/main.cpp

    src/model/core/Image.cpp
    src/model/core/Secret.cpp

    src/model/steganography/lsb/LSBEncoder.cpp
    src/model/steganography/dct/DCTEncoder.cpp
    src/model/steganography/dwt/DWTEncoder.cpp
    src/model/steganography/pvd/PVDEncoder.cpp

    src/model/compare/psnr/PSNR.cpp
    src/model/compare/ssim/SSIM.cpp

    src/controller/Controller.cpp

    src/view/cli/CLIParser.cpp
)

add_executable(main ${SOURCES})
target_link_libraries(main ${JPEG_LIBRARIES} OpenMP::OpenMP_CXX correct m)
