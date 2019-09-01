cmake_minimum_required(VERSION 3.4.3)
project(fuzzbuilder)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(inc)
add_definitions(${LLVM_DEFINITIONS})

add_executable(fuzzbuilder main.cc config.cc logger.cc execgen.cc seedgen.cc loader.cc irreader.cc irfreader.cc irireader.cc irwriter.cc)
target_link_libraries(fuzzbuilder PUBLIC LLVMBitReader LLVMBitWriter)
