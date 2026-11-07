# Copyright 2024 SSE4
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

if(NOT BOOST_IOSTREAMS_ENABLE_BROTLI)
  return()
endif()

if(NOT TARGET Boost::unit_test_framework)
  return()
endif()

add_executable(boost_iostreams_brotli_test brotli_test.cpp)

target_link_libraries(boost_iostreams_brotli_test
  PRIVATE Boost::iostreams Boost::unit_test_framework)

add_test(NAME boost_iostreams_brotli_test COMMAND boost_iostreams_brotli_test)
