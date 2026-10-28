cmake_minimum_required(VERSION 3.20)
project(ShardCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(shardcache_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
  endif()
endfunction()

#core library
add_library(shardcache_core
  src/cache/store.cpp
  src/net/socket.cpp
  src/net/line_reader.cpp
  src/protocol/protocol.cpp
  src/ring/hash_ring.cpp
  src/server/execute.cpp
  src/server/server.cpp
)
target_include_directories(shardcache_core PUBLIC src)
shardcache_warnings(shardcache_core)
# ws2_32-Winsock library
if(WIN32)
  target_link_libraries(shardcache_core PUBLIC ws2_32)
endif()

add_executable(shardcache_server src/server_main.cpp)
target_link_libraries(shardcache_server PRIVATE shardcache_core)
shardcache_warnings(shardcache_server)

add_executable(shardcache_cli src/cli_main.cpp)
target_link_libraries(shardcache_cli PRIVATE shardcache_core)
shardcache_warnings(shardcache_cli)

#tests (GoogleTest)
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
  tests/test_protocol.cpp
  tests/test_server.cpp
  tests/test_concurrency.cpp
  tests/test_hash_ring.cpp
)
target_link_libraries(shardcache_tests PRIVATE shardcache_core GTest::gtest_main)
shardcache_warnings(shardcache_tests)

gtest_discover_tests(shardcache_tests)
