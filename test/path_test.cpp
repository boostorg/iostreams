// (C) Copyright Daniel James 2011.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/detail/path.hpp>

#include <boost/core/enable_if.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/mpl/list.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#ifdef BOOST_IOSTREAMS_TEST_PATH_WITH_STD_FILESYSTEM
#include <filesystem>
#endif

template <typename Path>
void check_path_invariant(
    Path const& original_path,
    boost::iostreams::detail::path const& path,
    typename boost::enable_if<typename boost::is_same<typename Path::string_type::value_type, char>, void*>::type = NULL)
{
    BOOST_CHECK(!path.is_wide());
    BOOST_CHECK(original_path.native() == path.c_str());
    BOOST_CHECK(std::wstring() == path.c_wstr());
}

template <typename Path>
void check_path_invariant(
    Path const& original_path,
    boost::iostreams::detail::path const& path,
    typename boost::enable_if<typename boost::is_same<typename Path::string_type::value_type, wchar_t>, void*>::type = NULL)
{
    BOOST_CHECK(path.is_wide());
    BOOST_CHECK(original_path.native() == path.c_wstr());
    BOOST_CHECK(std::string() == path.c_str());
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(construct_from_std_filesystem_path_test, Path)
{
    Path const original_path("abc");

    boost::iostreams::detail::path const path(original_path);

    check_path_invariant(original_path, path);
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(assign_std_filesystem_path_test, Path)
{
    Path const original_path("abc");

    boost::iostreams::detail::path path;
    path = original_path;

    check_path_invariant(original_path, path);
}

typedef boost::mpl::list<
        boost::filesystem::path
#ifdef BOOST_IOSTREAMS_TEST_PATH_WITH_STD_FILESYSTEM
        , std::filesystem::path
#endif
    > path_types;

boost::unit_test::test_suite* init_unit_test_suite(int, char* [])
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE("path test");
    test->add(BOOST_TEST_CASE_TEMPLATE(construct_from_std_filesystem_path_test, path_types));
    test->add(BOOST_TEST_CASE_TEMPLATE(assign_std_filesystem_path_test, path_types));
    return test;
}
