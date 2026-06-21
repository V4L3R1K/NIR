cmake_minimum_required(VERSION 3.10)
project(libcorrect C)

set(CMAKE_C_STANDARD 99)

# Reed-Solomon sources
set(RS_SOURCES
    src/reed-solomon/polynomial.c
    src/reed-solomon/reed-solomon.c
    src/reed-solomon/encode.c
    src/reed-solomon/decode.c
)

add_library(correct STATIC ${RS_SOURCES})
target_include_directories(correct PUBLIC include)