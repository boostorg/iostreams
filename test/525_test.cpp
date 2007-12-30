// (C) Copyright Jonathan Turkanis 2004
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/code_converter.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/sequence.hpp"
#include "detail/temp_file.hpp"
#include "detail/utf8_codecvt_facet.hpp"
#include "detail/verification.hpp"

using boost::unit_test::test_suite;      
using namespace boost::iostreams;      
using namespace boost::iostreams::test;      

void array_test()
{
    char array[100];
    typedef code_converter<
                array_sink, 
                utf8_codecvt_facet<wchar_t, char>
            > MyArrayDevice;
    stream<MyArrayDevice> wCharOut(array, 100);
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("array test");
    test->add(BOOST_TEST_CASE(&array_test));
    return test;
}
