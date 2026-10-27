// (C) COPYRIGHT 2024 SSE4
// Based on zstd_test.cpp by:
// (C) COPYRIGHT 2018 Reimar Döffinger
// Based on lzma_test.cpp by:
// (C) COPYRIGHT 2017 ARM Limited
// Based on gzip_test.cpp by:
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Note: basically a copy-paste of the gzip test

#include <cstddef>
#include <string>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/brotli.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/ref.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/sequence.hpp"
#include "detail/verification.hpp"

using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
namespace io = boost::iostreams;
using boost::unit_test::test_suite;

template<class T> struct basic_test_alloc: std::allocator<T>
{
    basic_test_alloc()
    {
    }

    basic_test_alloc( basic_test_alloc const& /*other*/ )
    {
    }

    template<class U>
    basic_test_alloc( basic_test_alloc<U> const & /*other*/ )
    {
    }

    template<class U> struct rebind
    {
        typedef basic_test_alloc<U> other;
    };
};

typedef basic_test_alloc<char> brotli_allic;

void compression_test()
{
    text_sequence      data;

    // Test compression and decompression with custom allocator
    BOOST_CHECK(
        test_filter_pair( basic_brotli_compressor<brotli_allic>(),
                          basic_brotli_decompressor<brotli_allic>(),
                          std::string(data.begin(), data.end()) )
    );
}

void multiple_member_test()
{
    text_sequence      data;
    std::vector<char>  temp, dest;

    // Write compressed data to temp, twice in succession
    filtering_ostream out;
    out.push(brotli_compressor());
    out.push(io::back_inserter(temp));
    io::copy(make_iterator_range(data), out);
    out.push(io::back_inserter(temp));
    io::copy(make_iterator_range(data), out);
    BOOST_CHECK(std::equal(temp.begin(), temp.begin() + temp.size()/2, temp.begin() + temp.size()/2));

    // Read compressed data from temp into dest
    filtering_istream in;
    in.push(brotli_decompressor());
    in.push(array_source(&temp[0], temp.size()));
    io::copy(in, io::back_inserter(dest));

    // Check that dest consists of two copies of data
    BOOST_REQUIRE_EQUAL(data.size() * 2, dest.size());
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin()));
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin() + dest.size() / 2));

    dest.clear();
    io::copy(
        array_source(&temp[0], temp.size()),
        io::compose(brotli_decompressor(), io::back_inserter(dest)));

    // Check that dest consists of two copies of data
    BOOST_REQUIRE_EQUAL(data.size() * 2, dest.size());
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin()));
    BOOST_CHECK(std::equal(data.begin(), data.end(), dest.begin() + dest.size() / 2));
}

void array_source_test()
{
    std::string data = "simple test string.";
    std::string encoded;

    filtering_ostream out;
    out.push(brotli_compressor());
    out.push(io::back_inserter(encoded));
    io::copy(make_iterator_range(data), out);

    std::string res;
    io::array_source src(encoded.data(),encoded.length());
    io::copy(io::compose(io::brotli_decompressor(), src), io::back_inserter(res));

    BOOST_CHECK_EQUAL(data, res);
}

bool is_truncated_input(const brotli_error& e)
{
    return e.error() == brotli::truncated_input;
}

void truncated_input_test()
{
    // A brotli stream that ends in mid-member must be reported as an error
    // rather than silently decoding as though it were complete.
    text_sequence      data;
    std::vector<char>  temp, dest;

    filtering_ostream out;
    out.push(brotli_compressor());
    out.push(io::back_inserter(temp));
    io::copy(make_iterator_range(data), out);

    BOOST_REQUIRE_GT(temp.size(), 2u);
    temp.resize(temp.size() / 2);

    BOOST_CHECK_EXCEPTION(
        io::copy(
            array_source(&temp[0], temp.size()),
            io::compose(brotli_decompressor(), io::back_inserter(dest))),
        brotli_error,
        is_truncated_input);
}

void empty_file_test()
{
    // This test is in response to https://svn.boost.org/trac/boost/ticket/5237
    // The previous implementation of gzip_compressor only wrote the gzip file
    // header when the first bytes of uncompressed input were processed, causing
    // incorrect behavior for empty files
    BOOST_CHECK(
        test_filter_pair( brotli_compressor(),
                          brotli_decompressor(),
                          std::string() )
    );
}

void multipart_test()
{
    // This test verifies that the brotli_decompressor properly handles a file
    // that consists of multiple concatenated files (matches unbrotli behaviour)
    static const char multipart_file[] = {
        '\x1b', '\x14', '\x00', '\x00', '\xc4', '\x0d', '\xce', '\x13', '\x0a', '\xb4', '\x12', '\x4d',
        '\x34', '\x58', '\x4e', '\x06', '\x24', '\x99', '\x25', '\x92', '\xf7', '\x7e', '\x14', '\xb8',
        '\x00', '\x1b', '\x14', '\x00', '\x00', '\xc4', '\x0d', '\xce', '\x13', '\x4a', '\xa0', '\x12',
        '\x4a', '\x34', '\x58', '\x4e', '\x06', '\x24', '\x99', '\x25', '\x92', '\xf7', '\x7e', '\x14',
        '\xb8', '\x00', '\x1b', '\x14', '\x00', '\x00', '\xc4', '\x0d', '\xce', '\x13', '\x4a', '\xac',
        '\x02', '\x4f', '\x34', '\x58', '\x4e', '\x06', '\x24', '\x99', '\x25', '\x92', '\xf7', '\x7e',
        '\x14', '\xb8', '\x00'
    };

    filtering_istream in;
    std::string line;

    in.push(brotli_decompressor());
    in.push(io::array_source(multipart_file, sizeof(multipart_file)));

    // First part
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 1", line);
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 2", line);
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 3", line);

    // Second part immediately follows
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 4", line);
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 5", line);
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 6", line);

    // Then an empty part, followed by one last 3-line part.
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 7", line);
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 8", line);
    std::getline(in, line);
    BOOST_CHECK_EQUAL("Line 9", line);

    // Check for brotli errors too.
    BOOST_CHECK(!in.bad());
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("brotli test");
    test->add(BOOST_TEST_CASE(&compression_test));
    test->add(BOOST_TEST_CASE(&multiple_member_test));
    test->add(BOOST_TEST_CASE(&array_source_test));
    test->add(BOOST_TEST_CASE(&truncated_input_test));
    test->add(BOOST_TEST_CASE(&empty_file_test));
    test->add(BOOST_TEST_CASE(&multipart_test));
    return test;
}
