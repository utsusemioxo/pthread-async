cmake_minimum_required(VERSION 3.16)
project(pthread_async CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-fno-exceptions)
endif()

include_directories(include)

find_package(Threads REQUIRED)

add_executable(main src/main.cpp)
target_link_libraries(main Threads::Threads)

enable_testing()

add_executable(test_result tests/test_result.cpp)
target_link_libraries(test_result Threads::Threads)
add_test(NAME test_result COMMAND test_result)

add_executable(test_async tests/test_async.cpp)
target_link_libraries(test_async Threads::Threads)
add_test(NAME test_async COMMAND test_async)

add_executable(test_sdk_lifecycle tests/test_sdk_lifecycle.cpp)
target_link_libraries(test_sdk_lifecycle Threads::Threads)
add_test(NAME test_sdk_lifecycle COMMAND test_sdk_lifecycle)
