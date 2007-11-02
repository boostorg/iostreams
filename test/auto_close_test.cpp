// (C) Copyright Jonathan Turkanis 2004
// (C) Copyright Frank Birbacher 2007
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cstdio>  // EOF.
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/tee.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;

/* Implements observers for checking open state of device.
 * This class shall derive from the real device. The original
 * open and close methods are then hidden by the ones defined
 * here. Meant to be used only in this unit test.
 */
template<typename Base>
struct checker : Base
{
    checker() : open_(new bool(true)) { }
    void open() { *open_ = true; }
    void close() { if(!open_) throw 0; *open_ = false; }
    void close(std::ios_base::openmode)
        { close(); }
    bool is_open() const { return *open_; }
private:
    boost::shared_ptr<bool> open_;
};

/* A no-op observable source */
struct closable_source
    : public checker<source>
{
    std::streamsize read(char*, std::streamsize) { return 0; }
};

/* A no-op observable sink */
struct closable_sink
    : public checker<sink>
{
    std::streamsize write(const char*, std::streamsize) { return 0; }
};

/* A no-op observable bidirectional device */
struct Empty {}; //Helper
struct closable_bidir
    : public checker<Empty>
{
    typedef char char_type;
    struct category
        : closable_tag
        , bidirectional_device_tag
    {};

    std::streamsize read(char*, std::streamsize) { return 0; }
    std::streamsize write(const char*, std::streamsize) { return 0; }
};

/* A no-op observable tee device */
struct closable_tee_sink_base
{
    closable_sink s1, s2;
};
struct closable_tee_sink
	: public closable_tee_sink_base
	, public tee_device<closable_sink, closable_sink>
{
    closable_tee_sink()
        : closable_tee_sink_base() //first to construct
        , tee_device<closable_sink, closable_sink>(s1, s2)
    {}
    bool is_open() const { return s1.is_open() || s2.is_open(); }
    void open() { s1.open(); s2.open(); }
};

/* A no-op observable filter */
class closable_input_filter : public input_filter {
public:
    closable_input_filter() : open_(new bool(true)) { }

    template<typename Source>
    int get(Source&) { return EOF; }

    void open() { *open_ = true; }

    template<typename Source>
    void close(Source&) { *open_ = false; }

    bool is_open() const { return *open_; }
private:
    boost::shared_ptr<bool> open_;
};

////////////////////////////////////////////////////////////////////////////////

/* Test closing a device */
template<typename Device>
void auto_close()
{
    // Rely on auto_close to close source.
    Device dev;
    {
        stream<Device> strm(dev);
        BOOST_CHECK(dev.is_open());
        BOOST_CHECK(strm.auto_close());
    }
    BOOST_CHECK(!dev.is_open());

    // Use close() to close components.
    dev.open();
    {
        stream<Device> strm(dev);
        BOOST_CHECK(dev.is_open());
        BOOST_CHECK(strm.auto_close());
        strm.close();
        BOOST_CHECK(!dev.is_open());
    }

    // Use close() to close components, with auto_close disabled.
    dev.open();
    {
        stream<Device> strm(dev);
        BOOST_CHECK(dev.is_open());
        strm.set_auto_close(false);
        strm.close();
        BOOST_CHECK(!dev.is_open());
    }

    // Disable auto_close.
    dev.open();
    {
        stream<Device> strm(dev);
        BOOST_CHECK(dev.is_open());
        strm.set_auto_close(false);
        BOOST_CHECK(!strm.auto_close());
    }
    BOOST_CHECK(dev.is_open());
}

/* Test closing a filter */
void auto_close_filter()
{
    closable_source        src;
    closable_input_filter  flt;

    // Rely on auto_close to close components.
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        BOOST_CHECK(in.auto_close());
    }
    BOOST_CHECK(!flt.is_open());
    BOOST_CHECK(!src.is_open());

    // Use reset() to close components.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        BOOST_CHECK(in.auto_close());
        in.reset();
        BOOST_CHECK(!flt.is_open());
        BOOST_CHECK(!src.is_open());
    }

    // Use reset() to close components, with auto_close disabled.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        in.reset();
        BOOST_CHECK(!flt.is_open());
        BOOST_CHECK(!src.is_open());
    }

    // Disable auto_close.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        BOOST_CHECK(!in.auto_close());
        in.pop();
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
    }
    BOOST_CHECK(!flt.is_open());
    BOOST_CHECK(src.is_open());

    // Disable auto_close; disconnect and reconnect resource.
    flt.open();
    src.open();
    {
        filtering_istream in;
        in.push(flt);
        in.push(src);
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.set_auto_close(false);
        BOOST_CHECK(!in.auto_close());
        in.pop();
        BOOST_CHECK(flt.is_open());
        BOOST_CHECK(src.is_open());
        in.push(src);
    }
    BOOST_CHECK(!flt.is_open());
    BOOST_CHECK(!src.is_open());
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("auto_close test");
    test->add(BOOST_TEST_CASE(&auto_close<closable_source>));
    test->add(BOOST_TEST_CASE(&auto_close<closable_sink>));
    test->add(BOOST_TEST_CASE(&auto_close<closable_bidir>));
    test->add(BOOST_TEST_CASE(&auto_close<closable_tee_sink>));
    test->add(BOOST_TEST_CASE(&auto_close_filter));
    return test;
}
