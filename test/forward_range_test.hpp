// (C) Copyright 2026
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_FORWARD_RANGE_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_FORWARD_RANGE_HPP_INCLUDED

#include <cstddef>
#include <list>
#include <string>
#include <vector>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/test/test_tools.hpp>

namespace {

struct to_char {
    typedef char result_type;
    char operator()(int c) const { return static_cast<char>(c); }
};

struct proxy_char_ref {
    char* p;
    proxy_char_ref(char* p) : p(p) {}
    operator char() const { return *p; }
    proxy_char_ref& operator=(char c) { *p = c; return *this; }
    proxy_char_ref& operator=(const proxy_char_ref& other) { *p = *other.p; return *this; }
};

struct proxy_char_iterator {
    typedef std::forward_iterator_tag iterator_category;
    typedef char                      value_type;
    typedef std::ptrdiff_t            difference_type;
    typedef proxy_char_ref*           pointer;
    typedef proxy_char_ref            reference;

    char* ptr;

    proxy_char_iterator() : ptr(0) {}
    explicit proxy_char_iterator(char* p) : ptr(p) {}

    reference operator*() const { return proxy_char_ref(ptr); }
    proxy_char_iterator& operator++() { ++ptr; return *this; }
    proxy_char_iterator operator++(int) {
        proxy_char_iterator tmp = *this;
        ++ptr;
        return tmp;
    }
    bool operator==(const proxy_char_iterator& o) const { return ptr == o.ptr; }
    bool operator!=(const proxy_char_iterator& o) const { return ptr != o.ptr; }
};

} // anonymous namespace

void forward_range_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;

    // 1. Reading from forward range exceeding stream buffer size (4096)
    {
        vector<int> input(5000, 0x41);
        filtering_istream in(
            make_iterator_range(
                make_transform_iterator(input.begin(), to_char()),
                make_transform_iterator(input.end(), to_char())
            )
        );
        string output((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(output.size(), 5000);
        BOOST_CHECK(output == string(5000, 'A'));
    }

    // 2. Reading from forward range matching stream buffer size exactly (4096)
    {
        vector<int> input(4096, 0x42);
        filtering_istream in(
            make_iterator_range(
                make_transform_iterator(input.begin(), to_char()),
                make_transform_iterator(input.end(), to_char())
            )
        );
        string output((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(output.size(), 4096);
        BOOST_CHECK(output == string(4096, 'B'));
    }

    // 3. Reading from forward range smaller than buffer size
    {
        vector<int> input(100, 0x43);
        filtering_istream in(
            make_iterator_range(
                make_transform_iterator(input.begin(), to_char()),
                make_transform_iterator(input.end(), to_char())
            )
        );
        string output((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(output.size(), 100);
        BOOST_CHECK(output == string(100, 'C'));
    }

    // 4. Reading from empty forward range
    {
        vector<int> input;
        filtering_istream in(
            make_iterator_range(
                make_transform_iterator(input.begin(), to_char()),
                make_transform_iterator(input.end(), to_char())
            )
        );
        string output((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(output.size(), 0);
    }

    // 5. Direct multi-buffer reading with in.read() exceeding buffer size
    {
        vector<int> input(5000, 0x44);
        filtering_istream in(
            make_iterator_range(
                make_transform_iterator(input.begin(), to_char()),
                make_transform_iterator(input.end(), to_char())
            )
        );
        vector<char> buf(5000, ' ');
        in.read(&buf[0], 5000);
        BOOST_CHECK_EQUAL(in.gcount(), 5000);
        BOOST_CHECK(string(buf.begin(), buf.end()) == string(5000, 'D'));
    }

    // 6. Writing to forward range with excess capacity (unbuffered)
    {
        list<char> storage(10, ' ');
        filtering_ostream out(make_iterator_range(storage), 0);
        out.write("hello", 5);
        BOOST_CHECK(out.good());
        string s(storage.begin(), storage.end());
        BOOST_CHECK_EQUAL(s, "hello     ");
    }

    // 7. Writing to forward range with exact capacity (unbuffered)
    {
        list<char> storage(5, ' ');
        filtering_ostream out(make_iterator_range(storage), 0);
        out.write("hello", 5);
        BOOST_CHECK(out.good());
        string s(storage.begin(), storage.end());
        BOOST_CHECK_EQUAL(s, "hello");
    }

    // 8. Writing to forward range with insufficient capacity (unbuffered)
    {
        list<char> storage(4, ' ');
        filtering_ostream out(make_iterator_range(storage), 0);
        out.exceptions(std::ios_base::badbit);
        BOOST_CHECK_THROW(out.write("hello", 5), std::exception);
    }

    // 9. Writing to forward range exceeding stream buffer size (buffered)
    {
        list<char> storage(5000, ' ');
        string payload(5000, 'Z');
        filtering_ostream out(make_iterator_range(storage));
        out.write(payload.data(), payload.size());
        out.flush();
        BOOST_CHECK(out.good());
        string s(storage.begin(), storage.end());
        BOOST_CHECK_EQUAL(s, payload);
    }

    // 10. Writing to forward range with insufficient capacity (buffered)
    {
        list<char> storage(100, ' ');
        string payload(5000, 'Z');
        filtering_ostream out(make_iterator_range(storage));
        out.exceptions(std::ios_base::badbit);
        BOOST_CHECK_THROW({
            out.write(payload.data(), payload.size());
            out.flush();
        }, std::exception);
    }

    // 11. Reading and writing with custom forward iterator returning proxy reference
    {
        string src = "proxy iterator range adapter test";
        vector<char> src_vec(src.begin(), src.end());
        proxy_char_iterator rbegin(&src_vec[0]);
        proxy_char_iterator rend(&src_vec[0] + src_vec.size());

        filtering_istream in(make_iterator_range(rbegin, rend));
        string out_str((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        BOOST_CHECK_EQUAL(out_str, src);

        vector<char> dst_vec(src.size(), ' ');
        proxy_char_iterator wbegin(&dst_vec[0]);
        proxy_char_iterator wend(&dst_vec[0] + dst_vec.size());

        filtering_ostream out(make_iterator_range(wbegin, wend), 0);
        out.write(&src[0], src.size());
        BOOST_CHECK(out.good());
        BOOST_CHECK_EQUAL(string(dst_vec.begin(), dst_vec.end()), src);
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_FORWARD_RANGE_HPP_INCLUDED
