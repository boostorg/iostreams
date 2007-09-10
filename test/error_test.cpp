// (C) Copyright Frank Birbacher 2007
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/categories.hpp>  // tags.
#include <boost/iostreams/detail/ios.hpp>  // openmode, seekdir, int types.
#include <boost/iostreams/detail/error.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

namespace bio = boost::iostreams;
using boost::unit_test::test_suite;

/* This test unit uses a custom device to trigger errors. The device supports
 * input, output, and seek according to the SeekableDevice concept. And each
 * of the required functions throw a special detail::bad_xxx exception. This
 * should trigger the iostreams::stream to set the badbit status flag.
 * Additionally the exception can be propagated to the caller if the exception
 * mask of the stream allows exceptions.
 *
 * The stream offers four different functions: read, write, seekg, and seekp.
 * Each of them is tested with three different error reporting concepts:
 * test by reading status flags, test by propagated exception, and test by
 * calling std::ios_base::exceptions when badbit is already set.
 *
 * In each case all of the status checking functions of a stream are checked.
 */

// error device ***************************************************************
struct error_device
{
    typedef char   char_type;
    struct category
        : bio::seekable_device_tag,
          bio::closable_tag
        { };
    
    error_device(char const*) : is_open_flag(true) {}
    
    void open(char const*) { is_open_flag=true; }
    bool is_open() const { return is_open_flag; }
    void close() { is_open_flag=false; }
    
    std::streamsize read(char_type* s, std::streamsize n);
    std::streamsize write(const char_type* s, std::streamsize n);
    std::streampos seek(bio::stream_offset off, BOOST_IOS::seekdir way);
private:
    bool is_open_flag;
};

std::streamsize error_device::read(char_type*, std::streamsize)
{
    throw bio::detail::bad_read();
}

std::streamsize error_device::write(const char_type*, std::streamsize)
{
    throw bio::detail::bad_write();
}

std::streampos error_device::seek(bio::stream_offset, BOOST_IOS::seekdir)
{
    throw bio::detail::bad_seek();
}

// helping definitions ********************************************************
typedef bio::stream<error_device> test_stream;

void check_stream_for_badbit(std::iostream& stream)
{
    BOOST_REQUIRE_MESSAGE(!stream.good(), "stream still good");
    BOOST_CHECK_MESSAGE(!stream.eof(), "eofbit set but not expected");
    BOOST_CHECK_MESSAGE(stream.bad(), "stream did not set badbit");
    BOOST_CHECK_MESSAGE(stream.fail(), "stream did not fail");
    BOOST_CHECK_MESSAGE(stream.operator ! (),
            "stream does not report failure by operator !");
    BOOST_CHECK_MESSAGE(0 == stream.operator void* (),
            "stream does not report failure by operator void*");
}

// error checking concepts ****************************************************
template<void (*const function)(test_stream&)>
void wrap_nothrow()
{
    test_stream stream("foo");
    BOOST_REQUIRE_NO_THROW( function(stream) );
    check_stream_for_badbit(stream);
}

template<void (*const function)(test_stream&)>
void wrap_throw()
{
    typedef std::ios_base ios;
    test_stream stream("foo");
    
    BOOST_REQUIRE_NO_THROW( stream.exceptions(ios::failbit | ios::badbit) );
    BOOST_CHECK_THROW( function(stream), std::exception );
    
    check_stream_for_badbit(stream);
}

template<void (*const function)(test_stream&)>
void wrap_throw_delayed()
{
    typedef std::ios_base ios;
    test_stream stream("foo");
    
    BOOST_REQUIRE_NO_THROW( function(stream) );
    BOOST_CHECK_THROW(
            stream.exceptions(ios::failbit | ios::badbit),
            ios::failure
        );
    
    check_stream_for_badbit(stream);
}

// error raising **************************************************************
void test_read(test_stream& stream)
{
    char data[10];
    stream.read(data, 10);
}

void test_write_flush(test_stream& stream)
{
    char data[10] = {0};
    stream.write(data, 10);
        //force use of streambuf
    stream.flush();
}

void test_write_close(test_stream& stream)
{
    char data[10] = {0};
    stream.write(data, 10);
        //force use of streambuf
    stream.close();
}

void test_seekg(test_stream& stream)
{
    stream.seekg(10);
}

void test_seekp(test_stream& stream)
{
    stream.seekp(10);
}

// test registration function *************************************************
test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("error test");
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_read>));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_read>));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_read>));
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_write_flush>));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_write_flush>));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_write_flush>));
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_write_close>));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_write_close>));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_write_close>));
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_seekg>));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_seekg>));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_seekg>));
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_seekp>));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_seekp>));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_seekp>));
    
    return test;
}
