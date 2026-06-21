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

# libcorrect (Reed-Solomon)
set(LIBCORRECT_DIR ${CMAKE_SOURCE_DIR}/third_party/libcorrect)
set(LIBCORRECT_INCLUDE_DIR ${LIBCORRECT_DIR}/include)
set(LIBCORRECT_LIBRARY ${LIBCORRECT_DIR}/build/lib/libcorrect.a)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${JPEG_INCLUDE_DIR}
    ${LIBCORRECT_INCLUDE_DIR}
)

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
target_link_libraries(main ${JPEG_LIBRARIES} OpenMP::OpenMP_CXX ${LIBCORRECT_LIBRARY} m)