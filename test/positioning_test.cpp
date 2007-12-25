// (C) Copyright Jonathan Turkanis 2005
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/detail/ios.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/integer_traits.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using boost::unit_test::test_suite;

void large_file_test()
{
    stream_offset  large_file = (stream_offset) 100 *
                                (stream_offset) 1024 *
                                (stream_offset) 1024 *
                                (stream_offset) 1024;
    stream_offset  first = -large_file - (-large_file) % 10000000;
    stream_offset  last = large_file - large_file % 10000000;

    fstream log;
    log.open("C:/borland-output-5.8.2.txt", std::ios::out | std::ios::trunc);
    for (stream_offset off = first; off < last; off += 10000000)
    {                                           
        if (off == position_to_offset(offset_to_position(off))) {
                log << "offset = " << off << "\n";
        } else {
                log << "offset = " << off
                    << "; pos = " << position_to_offset(offset_to_position(off))
                    << "\n";
        }
        //BOOST_CHECK(off == position_to_offset(offset_to_position(off)));
    }
}

void small_file_test()
{
    stream_offset  small_file = 1000000;
    stream_offset  off = -small_file;
    streampos      pos = offset_to_position(off);

    while (off < small_file)
    {
        BOOST_CHECK(off == position_to_offset(offset_to_position(off)));
        BOOST_CHECK(pos == offset_to_position(position_to_offset(pos)));
        off += 20000;
        pos += 20000;
        BOOST_CHECK(off == position_to_offset(offset_to_position(off)));
        BOOST_CHECK(pos == offset_to_position(position_to_offset(pos)));
        off -= 10000;
        pos -= 10000;
    }
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("positioning test");
    test->add(BOOST_TEST_CASE(&large_file_test));
    test->add(BOOST_TEST_CASE(&small_file_test));
    return test;
}
