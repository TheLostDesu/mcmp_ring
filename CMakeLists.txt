cmake_minimum_required(VERSION 3.20)
project(mpmc_ring LANGUAGES CXX)

# Destructive interference size of the *target* CPU. 64 is right for x86-64
# and most AArch64 server cores; Apple M-series uses 128.
set(MPMC_RING_CACHE_LINE_SIZE 64 CACHE STRING
    "Cache line size of the target CPU in bytes (power of two)")
if(NOT MPMC_RING_CACHE_LINE_SIZE MATCHES "^(16|32|64|128|256)$")
  message(FATAL_ERROR
    "MPMC_RING_CACHE_LINE_SIZE must be one of 16, 32, 64, 128, 256; "
    "got '${MPMC_RING_CACHE_LINE_SIZE}'")
endif()

add_library(mpmc_ring INTERFACE)
add_library(mpmc_ring::mpmc_ring ALIAS mpmc_ring)
target_include_directories(mpmc_ring INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(mpmc_ring INTERFACE cxx_std_20)
# INTERFACE on purpose: the library is header-only, so every consumer must be
# compiled with the same value.
target_compile_definitions(mpmc_ring INTERFACE
  MPMC_RING_CACHE_LINE_SIZE=${MPMC_RING_CACHE_LINE_SIZE})

option(MPMC_RING_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
if(MPMC_RING_BUILD_TESTS)
  enable_testing()

  add_executable(config_test tests/config_test.cpp)
  target_link_libraries(config_test PRIVATE mpmc_ring::mpmc_ring)
  set_target_properties(config_test PROPERTIES CXX_EXTENSIONS OFF)
  target_compile_options(config_test PRIVATE -Wall -Wextra -Wpedantic -Werror)
  add_test(NAME config_test COMMAND config_test)
endif()
