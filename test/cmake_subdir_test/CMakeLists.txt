# Copyright 2018, 2019, 2021 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.5...3.16)

project(boost_iostreams_subdir_test LANGUAGES CXX)

if(BOOST_RUNTIME_LINK STREQUAL "static")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Put .dll in the same directory as .exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(BOOST_INCLUDE_LIBRARIES iostreams)

add_subdirectory(../../../../ deps/boost)

add_executable(test_fd test_fd.cpp)
target_link_libraries(test_fd Boost::iostreams Boost::core)

enable_testing()
add_test(NAME test_fd COMMAND test_fd WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(check VERBATIM COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --no-tests=error -C $<CONFIG>)
