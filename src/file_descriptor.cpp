// (C) Copyright Jonathan Turkanis 2003.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Inspired by fdstream.hpp, (C) Copyright Nicolai M. Josuttis 2001,
// available at http://www.josuttis.com/cppcode/fdstream.html.

// Define BOOST_IOSTREAMS_SOURCE so that <boost/iostreams/detail/config.hpp>
// knows that we are building the library (possibly exporting code), rather
// than using it (possibly importing code).
#define BOOST_IOSTREAMS_SOURCE 

#include <boost/config.hpp> // BOOST_JOIN
#include <boost/iostreams/detail/error.hpp>
#include <boost/iostreams/detail/config/dyn_link.hpp>
#include <boost/iostreams/detail/ios.hpp>  // openmodes, failure.
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/limits.hpp>

    // OS-specific headers for low-level i/o.

#include <cassert>
#include <cstdio>        // SEEK_SET, etc.
#include <errno.h>
#include <fcntl.h>       // file opening flags.
#include <sys/stat.h>    // file access permissions.
#ifdef BOOST_IOSTREAMS_WINDOWS
# include <io.h>         // low-level file i/o.
# define WINDOWS_LEAN_AND_MEAN
# include <windows.h>
#else
# include <sys/types.h>  // mode_t.
# include <unistd.h>     // low-level file i/o.
#endif

// Names of runtime library routines vary.
#if defined(__BORLANDC__)
# define BOOST_RTL(x) BOOST_JOIN(_rtl_, x)
#else
# if defined(BOOST_IOSTREAMS_WINDOWS) && !defined(__CYGWIN__)
#  define BOOST_RTL(x) BOOST_JOIN(_, x)
# else
#  define BOOST_RTL(x) ::x
# endif
#endif

namespace boost { namespace iostreams {

//------------------Implementation of file_descriptor-------------------------//

void file_descriptor::open
    ( const std::string& path, BOOST_IOS::openmode m,
      BOOST_IOS::openmode base )
{
    using namespace std;
    m |= base;
#ifdef BOOST_IOSTREAMS_WINDOWS //---------------------------------------------//
    DWORD dwDesiredAccess;
    DWORD dwCreationDisposition;
    if ( (m & (BOOST_IOS::in | BOOST_IOS::out)) 
             == 
         (BOOST_IOS::in | BOOST_IOS::out) ) 
    {
        assert(!(m & BOOST_IOS::app));
        dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        dwCreationDisposition = 
            (m & BOOST_IOS::trunc) ?
                OPEN_ALWAYS :
                OPEN_EXISTING;
    } else if (m & BOOST_IOS::in) {
        assert(!(m & (BOOST_IOS::app |BOOST_IOS::trunc)));
        dwDesiredAccess = GENERIC_READ;
        dwCreationDisposition = OPEN_EXISTING;
    } else if (m & BOOST_IOS::out) {
        dwDesiredAccess = GENERIC_WRITE;
        dwCreationDisposition = OPEN_ALWAYS;
        if (m & BOOST_IOS::app)
            pimpl_->flags_ |= impl::append;
    }

    HANDLE handle = 
        ::CreateFileA( path.c_str(),
                       dwDesiredAccess, 
                       0,                      // dwShareMode
                       NULL,                   // lpSecurityAttributes
                       dwCreationDisposition,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL );                 // hTemplateFile
    if (handle != INVALID_HANDLE_VALUE) {
        pimpl_->handle_ = handle;
        pimpl_->flags_ |= impl::close_on_exit | impl::has_handle;
    } else {
        pimpl_->flags_ = 0;
        throw BOOST_IOSTREAMS_FAILURE("bad open");
    }
#else // #ifdef BOOST_IOSTREAMS_WINDOWS //------------------------------------//

        // Calculate oflag argument to open.

    int oflag = 0;
    if ( (m & (BOOST_IOS::in | BOOST_IOS::out)) 
             == 
         (BOOST_IOS::in | BOOST_IOS::out) ) 
    {
        assert(!(m & BOOST_IOS::app));
        oflag |= O_RDWR;
    } else if (m & BOOST_IOS::in) {
        assert(!(m & (BOOST_IOS::app |BOOST_IOS::trunc)));
        oflag |= O_RDONLY;
    } else if (m & BOOST_IOS::out) {
        oflag |= O_WRONLY;
        m |= BOOST_IOS::trunc;
        if (m & BOOST_IOS::app)
            oflag |= O_APPEND;
    }
    if (m & BOOST_IOS::trunc)
        oflag |= O_CREAT;

        // Calculate pmode argument to open.

    mode_t pmode = S_IRUSR | S_IWUSR |
                   S_IRGRP | S_IWGRP |
                   S_IROTH | S_IWOTH;

        // Open file.

    int fd = BOOST_RTL(open)(path.c_str(), oflag, pmode);
    if (fd == -1) {
        throw BOOST_IOSTREAMS_FAILURE("bad open");
    } else {
        pimpl_->fd_ = fd;
        pimpl_->flags_ = impl::close_on_exit;
    }
#endif // #ifndef BOOST_IOSTREAMS_WINDOWS //----------------------------------//
}

std::streamsize file_descriptor::read(char_type* s, std::streamsize n)
{
#ifdef BOOST_IOSTREAMS_WINDOWS
    if (pimpl_->flags_ & impl::has_handle) {
        DWORD result;
        if (!::ReadFile(pimpl_->handle_, s, n, &result, NULL))
            throw detail::bad_read();
        return static_cast<std::streamsize>(result);
    }
#endif
    errno = 0;
    std::streamsize result = BOOST_RTL(read)(pimpl_->fd_, s, n);
    if (errno != 0)
        throw detail::bad_read();
    return result;
}

void file_descriptor::write(const char_type* s, std::streamsize n)
{
#ifdef BOOST_IOSTREAMS_WINDOWS
    if (pimpl_->flags_ & impl::has_handle) {
        if (pimpl_->flags_ & impl::append) {
            ::SetFilePointer(pimpl_->handle_, 0, NULL, FILE_END);
            if (::GetLastError() != NO_ERROR)
                throw detail::bad_seek();
        }
        DWORD ignore;
        if (!::WriteFile(pimpl_->handle_, s, n, &ignore, NULL))
            throw detail::bad_write();
        return;
    }
#endif
    int result = BOOST_RTL(write)(pimpl_->fd_, s, n);
    if (result < n)
        throw detail::bad_write();
}

boost::intmax_t file_descriptor::seek
    (boost::intmax_t off, BOOST_IOS::seekdir way)
{
    using namespace std;
#ifdef BOOST_IOSTREAMS_WINDOWS
    if (pimpl_->flags_ & impl::has_handle) {
        LONG lDistanceToMove = static_cast<LONG>(off & 0xffffffff);
        LONG lDistanceToMoveHigh = 
            off < 0xffffffff ? 
                static_cast<LONG>(off >> 32) :
                0;
        DWORD dwResultLow = 
            ::SetFilePointer( pimpl_->handle_,
                              lDistanceToMove,
                              &lDistanceToMoveHigh,
                              way == BOOST_IOS::beg ?
                                  FILE_BEGIN :
                                  way == BOOST_IOS::cur ?
                                    FILE_CURRENT :
                                    FILE_END );
        if (::GetLastError() != NO_ERROR) {
            throw detail::bad_seek();
        } else {
            return (static_cast<boost::intmax_t>(lDistanceToMoveHigh) << 32) + 
                   dwResultLow;
        }
    }
#endif
    if ( off > (numeric_limits<long>::max)() ||
         off < (numeric_limits<long>::min)() )
    {
        throw BOOST_IOSTREAMS_FAILURE("bad offset");
    }
    std::streamoff result =
        #if !defined(__BORLANDC__)
            BOOST_RTL(lseek)
        #else
            lseek
        #endif
            ( pimpl_->fd_, 
              static_cast<long>(off), 
              way == BOOST_IOS::beg ?
                  SEEK_SET :
                      way == BOOST_IOS::cur ?
                          SEEK_CUR :
                          SEEK_END );
    if (result == -1)
        throw detail::bad_seek();
    return result;
}

void file_descriptor::close() { close_impl(*pimpl_); }

void file_descriptor::close_impl(impl& i)
{
#ifdef BOOST_IOSTREAMS_WINDOWS
    if (i.flags_ & impl::has_handle) {
        if (!::CloseHandle(i.handle_))
            throw BOOST_IOSTREAMS_FAILURE("bad close");
        i.fd_ = -1;
        i.flags_ = 0;
        return;
    } 
#endif
    if (i.fd_ != -1) {
        if (BOOST_RTL(close)(i.fd_) == -1)
            throw BOOST_IOSTREAMS_FAILURE("bad close");
        i.fd_ = -1;
        i.flags_ = 0;
    }
}

} } // End namespaces iostreams, boost.
