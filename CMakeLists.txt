cmake_minimum_required(VERSION 3.20)
project(ShardCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# /W4: high warning level. /permissive-: strict standards conformance.
# /utf-8: treat source files as UTF-8.
function(shardcache_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
  endif()
endfunction()

# Core library
add_library(shardcache_core
  src/cache/store.cpp
)
target_include_directories(shardcache_core PUBLIC src)
shardcache_warnings(shardcache_core)

# Small demo program
add_executable(shardcache_demo src/main.cpp)
target_link_libraries(shardcache_demo PRIVATE shardcache_core)
shardcache_warnings(shardcache_demo)

# --- Tests (GoogleTest)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# MSVC pitfall
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_executable(shardcache_tests
  tests/test_store.cpp
)
target_link_libraries(shardcache_tests PRIVATE shardcache_core GTest::gtest_main)
shardcache_warnings(shardcache_tests)
# Registers every TEST in the executable with CTest individually.
gtest_discover_tests(shardcache_tests)
