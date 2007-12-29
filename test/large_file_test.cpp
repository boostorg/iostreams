/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Tests seeking with a file_descriptor using large file offsets.
 *
 * File:        libs/iostreams/test/large_file_test.cpp
 * Date:        Tue Dec 25 21:34:47 MST 2007
 * Copyright:   2007 CodeRage
 * Author:      Jonathan Turkanis
 */

#include <cstdio>            // SEEK_SET, etc.
#include <boost/config.hpp>  // BOOST_STRINGIZE
#include <boost/iostreams/detail/config/rtl.hpp>
#include <boost/iostreams/detail/config/windows_posix.hpp>
#include <boost/iostreams/detail/execute.hpp>
#include <boost/iostreams/detail/ios.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

    // OS-specific headers for low-level i/o.

#include <fcntl.h>       // file opening flags.
#include <sys/stat.h>    // file access permissions.
#ifdef BOOST_IOSTREAMS_WINDOWS
# include <io.h>         // low-level file i/o.
# define WINDOWS_LEAN_AND_MEAN
# include <windows.h>
# ifndef INVALID_SET_FILE_POINTER
#  define INVALID_SET_FILE_POINTER ((DWORD)-1)
# endif
#else
# include <sys/types.h>  // mode_t.
# include <unistd.h>     // low-level file i/o.
#endif  

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using boost::unit_test::test_suite;

//------------------Definition of constants-----------------------------------//

const stream_offset gigabyte = 1073741824;
const stream_offset file_size = // Some compilers complain about "8589934593"
    gigabyte * static_cast<stream_offset>(8) + static_cast<stream_offset>(1);
const int offset_list[] = 
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, // Seek by 1GB
      0, 2, 1, 3, 2, 4, 3, 5, 4, 6, 5, 7, 6, 8,       // Seek by 2GB
         6, 7, 5, 6, 4, 5, 3, 4, 2, 3, 1, 2, 
      0, 3, 1, 4, 2, 5, 3, 6, 4, 7, 5, 8,             // Seek by 3GB
         5, 7, 4, 6, 3, 5, 2, 4, 1,
      0, 4, 1, 5, 2, 6, 3, 7, 4, 8,                   // Seek by 4GB
         4, 7, 3, 6, 2, 5, 1, 4,
      0, 5, 1, 6, 2, 7, 3, 8, 3, 7, 2, 6, 1, 5,       // Seek by 5GB
      0, 6, 1, 7, 2, 8, 2, 7, 1, 6,                   // Seek by 6GB
      0, 7, 1, 8, 1, 7,                               // Seek by 7GB
      0, 8, 0 };                                      // Seek by 8GB
const int offset_list_length = sizeof(offset_list) / sizeof(int);
#ifdef LARGE_FILE_TEMP
    const char* file_name = BOOST_STRINGIZE(LARGE_FILE_TEMP);
    const bool keep_file = false;
#else
    const char* file_name = BOOST_STRINGIZE(LARGE_FILE_KEEP);
    const bool keep_file = true;
#endif

//------------------Definition of remove_large_file---------------------------//

// Removes the large file
void remove_large_file()
{
#ifdef BOOST_IOSTREAMS_WINDOWS
    DeleteFile(TEXT(file_name));
#else
    unlink(file_name);
#endif
}

//------------------Definition of create_large_file---------------------------//

// Creates and initializes the large file if it does not already exist
bool create_large_file()
{
#ifdef BOOST_IOSTREAMS_WINDOWS

    // If file exists and has correct size, we're done
    WIN32_FIND_DATA info;
    HANDLE hnd = FindFirstFile(TEXT(file_name), &info);
    if (hnd != INVALID_HANDLE_VALUE) {
        FindClose(hnd);
        __int64 size = 
            (static_cast<__int64>(info.nFileSizeHigh) << 32) + 
            static_cast<__int64>(info.nFileSizeLow);
        if (size == file_size)
            return true;
        else
            remove_large_file();
    }

    // Create file
    hnd =
        CreateFile(
            TEXT(file_name),
            GENERIC_WRITE,
            0,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | 
                FILE_FLAG_RANDOM_ACCESS |
                FILE_FLAG_WRITE_THROUGH,
            NULL
        );
    if (!hnd)
        return false;

    // Set file pointer
    LARGE_INTEGER length;
    length.HighPart = 0;
    length.LowPart = 0;
    length.QuadPart = file_size;
    if (!SetFilePointerEx(hnd, length, NULL, FILE_BEGIN)) {
        CloseHandle(hnd);
        remove_large_file();
        return false;
    }

    // Set file size
    if (!SetEndOfFile(hnd)) {
        CloseHandle(hnd);
        remove_large_file();
        return false;
    }

    // Initialize file data
    for (int z = 0; z <= 8; ++z) {

        // Seek
        length.HighPart = 0;
        length.LowPart = 0;
        length.QuadPart = z * gigabyte;
        LARGE_INTEGER ptr;
        if ( !SetFilePointerEx(hnd, length, &ptr, FILE_BEGIN) ||
             ptr.QuadPart != z * gigabyte )
        {
            CloseHandle(hnd);
            remove_large_file();
            return false;
        }

        // Write a character
        char buf[1] = { z + 1 };
        DWORD result;
        BOOL success = WriteFile(hnd, buf, 1, &result, NULL);
        if (!success || result != 1) {
            CloseHandle(hnd);
            remove_large_file();
            return false;
        }
    }

    // Close file
    CloseHandle(hnd);
	return true;

#else // #ifdef BOOST_IOSTREAMS_WINDOWS

    // If file exists and has correct size, we're done
    struct stat info;
    if (!stat(file_name, &info)) {
        if (info.st_size == file_size)
            return true;
        else
            remove_large_file();
    }

    // Create file
    int oflag = O_WRONLY | O_CREAT;
    #ifdef _LARGEFILE64_SOURCE
        oflag |= O_LARGEFILE;
    #endif
    mode_t pmode = 
        S_IRUSR | S_IWUSR |
        S_IRGRP | S_IWGRP |
        S_IROTH | S_IWOTH;
    int fd = BOOST_IOSTREAMS_FD_OPEN(file_name, oflag, pmode);
    if (fd == -1)
        return false;

    // Set file size
    if (BOOST_IOSTREAMS_FD_TRUNCATE(fd, file_size)) {
        BOOST_IOSTREAMS_FD_CLOSE(fd);
        return false;
    }

    // Initialize file data
    for (int z = 0; z <= 8; ++z) {

        // Seek
        BOOST_IOSTREAMS_FD_OFFSET off = 
            BOOST_IOSTREAMS_FD_SEEK(
                fd,
                static_cast<BOOST_IOSTREAMS_FD_OFFSET>(z * gigabyte),
                SEEK_SET
            );
        if (off == -1) {
            BOOST_IOSTREAMS_FD_CLOSE(fd);
            return false;
        }

        // Write a character
        char buf[1] = { z + 1 };
        if (BOOST_IOSTREAMS_FD_WRITE(fd, buf, 1) == -1) {
            BOOST_IOSTREAMS_FD_CLOSE(fd);
            return false;
        }
    }

    // Close file
    BOOST_IOSTREAMS_FD_CLOSE(fd);
	return true;
#endif // #ifdef BOOST_IOSTREAMS_WINDOWS
}

//------------------Definition of large_file----------------------------------//

// RAII utility
class large_file {
public:
    large_file() { exists_ = create_large_file(); }
    ~large_file() { if (!keep_file) remove_large_file(); }
    bool exists() const { return exists_; }
    const char* path() const { return file_name; }
private:
    bool exists_;
};
                    
//------------------Definition of check_character-----------------------------//

// Verify that the given file contains the given character at the current 
// position
bool check_character(file_descriptor_source& file, char value)
{
    char           buf[1];
    int            amt;
    BOOST_CHECK_NO_THROW(amt = file.read(buf, 1));
    BOOST_CHECK_MESSAGE(amt == 1, "failed reading character");
    BOOST_CHECK_NO_THROW(file.seek(-1, BOOST_IOS::cur));
    return buf[0] == value;
}

//------------------Definition of large_file_test-----------------------------//

void large_file_test()
{
    BOOST_REQUIRE_MESSAGE(
        sizeof(stream_offset) >= 64,
        "large offsets not supported"
    );

    // Prepare file and file descriptor
    large_file              large;
    file_descriptor_source  file;
    BOOST_CHECK_MESSAGE(large.exists(), "failed creating file");
    BOOST_CHECK_NO_THROW(file.open(large.path(), BOOST_IOS::binary));

    // Test seeking using ios_base::beg
    for (int z = 0; z < offset_list_length; ++z) {
        char value = offset_list[z] + 1;
        stream_offset off = 
            static_cast<stream_offset>(offset_list[z]) * gigabyte;
        BOOST_CHECK_NO_THROW(file.seek(off, BOOST_IOS::beg));
        BOOST_CHECK_MESSAGE(
            check_character(file, value), 
            "failed validating seek"
        );
    }

    // Test seeking using ios_base::end
    for (int z = 0; z < offset_list_length; ++z) {
        char value = offset_list[z] + 1;
        stream_offset off = 
            -static_cast<stream_offset>(8 - offset_list[z]) * gigabyte - 1;
        BOOST_CHECK_NO_THROW(file.seek(off, BOOST_IOS::end));
        BOOST_CHECK_MESSAGE(
            check_character(file, value), 
            "failed validating seek"
        );
    }

    // Test seeking using ios_base::cur
    for (int next, cur = 0, z = 0; z < offset_list_length; ++z, cur = next) {
        next = offset_list[z];
        char value = offset_list[z] + 1;
        stream_offset off = static_cast<stream_offset>(next - cur) * gigabyte;
        BOOST_CHECK_NO_THROW(file.seek(off, BOOST_IOS::cur));
        BOOST_CHECK_MESSAGE(
            check_character(file, value), 
            "failed validating seek"
        );
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("execute test");
    test->add(BOOST_TEST_CASE(&large_file_test));
    return test;
}
