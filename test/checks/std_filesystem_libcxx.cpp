// Copyright John Doe 2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include <filesystem>

#ifndef _LIBCPP_VERSION
#error libc++ expected
#endif

int main()
{
    std::filesystem::create_directory(std::filesystem::path("/tmp/cpp_lib_filesystem_link"));
}
