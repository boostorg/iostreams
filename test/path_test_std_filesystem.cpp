// (C) Copyright Daniel James 2011.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/detail/path.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <type_traits>

template <typename Path>
void check_path_invariant(
    Path const& original_path,
    boost::iostreams::detail::path const& path,
    std::enable_if_t<std::is_same_v<typename Path::string_type::value_type, char>, void*> = nullptr)
{
    BOOST_CHECK(!path.is_wide());
    BOOST_CHECK(original_path.native() == path.c_str());
    BOOST_CHECK(std::wstring() == path.c_wstr());
}

template <typename Path>
void check_path_invariant(
    Path const& original_path,
    boost::iostreams::detail::path const& path,
    std::enable_if_t<std::is_same_v<typename Path::string_type::value_type, wchar_t>, void*> = nullptr)
{
    BOOST_CHECK(path.is_wide());
    BOOST_CHECK(original_path.native() == path.c_wstr());
    BOOST_CHECK(std::string() == path.c_str());
}

void path_test_construct_from_std_filesystem_path()
{
    std::filesystem::path const original_path("abc");
    
    boost::iostreams::detail::path const path(original_path);
    
    check_path_invariant<std::filesystem::path>(original_path, path);
}

void path_test_assign_std_filesystem_path()
{
    std::filesystem::path const original_path("abc");
    
    boost::iostreams::detail::path path;
    path = original_path;
    
    check_path_invariant<std::filesystem::path>(original_path, path);
}

boost::unit_test::test_suite* init_unit_test_suite(int, char* []) 
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE("path_test_std_filesystem test");
    test->add(BOOST_TEST_CASE(&path_test_construct_from_std_filesystem_path));
    test->add(BOOST_TEST_CASE(&path_test_assign_std_filesystem_path));
    return test;
}
