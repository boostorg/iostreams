// (C) Copyright SSE4 2024.
// Based on zstd.cpp by:
// (C) Copyright Reimar Döffinger 2018.
// Based on lzma.cpp by:
// (C) Copyright Milan Svoboda 2008.
// Originally developed under the fusecompress project.
// Based on bzip2.cpp by:
// (C) Copyright Jonathan Turkanis 2003.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

// Define BOOST_IOSTREAMS_SOURCE so that <boost/iostreams/detail/config.hpp>
// knows that we are building the library (possibly exporting code), rather
// than using it (possibly importing code).
#define BOOST_IOSTREAMS_SOURCE

#include <brotli/decode.h>
#include <brotli/encode.h>

#include <boost/throw_exception.hpp>
#include <boost/iostreams/detail/config/dyn_link.hpp>
#include <boost/iostreams/filter/brotli.hpp>

namespace boost { namespace iostreams {

namespace brotli {
                    // Compression levels

const uint32_t best_speed           = BROTLI_MIN_QUALITY;
const uint32_t best_compression     = BROTLI_MAX_QUALITY;
const uint32_t default_compression  = BROTLI_DEFAULT_QUALITY;

                    // Status codes

const int okay                 = 0;
const int stream_end           = 1;

                    // Error codes

// Positive, so that it cannot collide with a BrotliDecoderErrorCode,
// all of which are zero or negative.
const int truncated_input      = 1;

                    // Flush codes

const int finish               = BROTLI_OPERATION_FINISH;
const int flush                = BROTLI_OPERATION_FLUSH;
const int run                  = BROTLI_OPERATION_PROCESS;
} // End namespace brotli.

//------------------Implementation of brotli_error------------------------------//

namespace {

const char* brotli_error_string(int error)
{
    if (error == brotli::truncated_input)
        return "brotli: unexpected end of input";
    return BrotliDecoderErrorString(static_cast<BrotliDecoderErrorCode>(error));
}

} // End anonymous namespace.

brotli_error::brotli_error(int error)
    : BOOST_IOSTREAMS_FAILURE(brotli_error_string(error)), error_(error)
    { }

void brotli_error::check BOOST_PREVENT_MACRO_SUBSTITUTION(int error)
{
    switch (static_cast<BrotliDecoderErrorCode>(error)) {
        case BROTLI_DECODER_ERROR_ALLOC_CONTEXT_MODES:
        case BROTLI_DECODER_ERROR_ALLOC_TREE_GROUPS:
        case BROTLI_DECODER_ERROR_ALLOC_CONTEXT_MAP:
        case BROTLI_DECODER_ERROR_ALLOC_RING_BUFFER_1:
        case BROTLI_DECODER_ERROR_ALLOC_RING_BUFFER_2:
        case BROTLI_DECODER_ERROR_ALLOC_BLOCK_TYPE_TREES:
            boost::throw_exception(std::bad_alloc());
        default:
            boost::throw_exception(brotli_error(error));
    }
}

//------------------Implementation of brotli_base-------------------------------//

namespace detail {

brotli_base::brotli_base()
    : encoder_(nullptr), decoder_(nullptr),
      avail_in_(0), avail_out_(0),
      next_in_(nullptr), next_out_(nullptr),
      level_(brotli::default_compression), eof_(false),
      alloc_(nullptr), free_(nullptr), opaque_(nullptr)
    { }

brotli_base::~brotli_base()
{
}

void brotli_base::before( const char*& src_begin, const char* src_end,
                        char*& dest_begin, char* dest_end )
{
    next_in_ = reinterpret_cast<const uint8_t*>(src_begin);
    avail_in_ = static_cast<size_t>(src_end - src_begin);
    next_out_ = reinterpret_cast<uint8_t*>(dest_begin);
    avail_out_ = static_cast<size_t>(dest_end - dest_begin);
}

void brotli_base::after(const char*& src_begin, char*& dest_begin, bool)
{
    src_begin = reinterpret_cast<const char*>(next_in_);
    dest_begin = reinterpret_cast<char*>(next_out_);
}

int brotli_base::deflate(int action)
{
    BrotliEncoderState* s = static_cast<BrotliEncoderState*>(encoder_);

    // brotli::flush and brotli::finish may need more than one call to drain
    // the encoder.  Give up as soon as the output buffer is full, so that the
    // caller gets a chance to hand us an empty one; spinning here would loop
    // forever, since brotli cannot make progress with no room to write.
    do {

    if (!BrotliEncoderCompressStream(s,
                                     static_cast<BrotliEncoderOperation>(action),
                                     &avail_in_,
                                     &next_in_,
                                     &avail_out_,
                                     &next_out_,
                                     nullptr))
        boost::throw_exception(std::bad_alloc());

    } while (action != brotli::run && avail_out_ &&
             (avail_in_ || BrotliEncoderHasMoreOutput(s)));

    return BrotliEncoderIsFinished(s) ? brotli::stream_end : brotli::okay;
}

int brotli_base::inflate(int action)
{
    // Ignore spurious extra calls.
    // Note size > 0 will trigger an error in this case.
    if (eof_) {
        if (avail_in_ == 0) {
            return brotli::stream_end;
        } else {
            reset(false, true);
        }
    }

    do {

        BrotliDecoderResult result = BrotliDecoderDecompressStream(static_cast<BrotliDecoderState *>(decoder_),
                                                                   &avail_in_,
                                                                   &next_in_,
                                                                   &avail_out_,
                                                                   &next_out_,
                                                                   nullptr);

        switch (result) {
            case BROTLI_DECODER_RESULT_ERROR:
                brotli_error::check BOOST_PREVENT_MACRO_SUBSTITUTION(
                        BrotliDecoderGetErrorCode(static_cast<BrotliDecoderState *>(decoder_)));
                break;
            case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT:
            case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT:
            case BROTLI_DECODER_RESULT_SUCCESS:
                break;
        }
    }
    while (BrotliDecoderHasMoreOutput(static_cast<BrotliDecoderState *>(decoder_)) && avail_out_);

    eof_ = BrotliDecoderIsFinished(static_cast<BrotliDecoderState *>(decoder_));

    if (eof_)
        return brotli::stream_end;

    // Not finished.  If the decoder still has buffered output, or we simply
    // ran out of room to write it, the caller will come back with a fresh
    // buffer and we can make progress then.
    if (avail_out_ == 0 ||
        BrotliDecoderHasMoreOutput(static_cast<BrotliDecoderState *>(decoder_)))
        return brotli::okay;

    // Otherwise the decoder wants more input.  If the caller has told us the
    // input is exhausted, the stream ended in mid-member.
    if (action == brotli::finish && avail_in_ == 0)
        boost::throw_exception(brotli_error(brotli::truncated_input));

    return brotli::okay;
}

void brotli_base::reset(bool compress, bool realloc)
{
    compress ?
    BrotliEncoderDestroyInstance(static_cast<BrotliEncoderState *>(encoder_)) :
    BrotliDecoderDestroyInstance(static_cast<BrotliDecoderState*>(decoder_));

    if (realloc) {
        init_stream(compress, alloc_, free_, opaque_);
    }
}

void brotli_base::do_init
    ( const brotli_params& p, bool compress,
      brotli::alloc_func alloc, brotli::free_func free,
      void* opaque)
{
    level_ = p.level;
    init_stream(compress, alloc, free, opaque);
}

void brotli_base::init_stream(bool compress,
                              brotli::alloc_func alloc, brotli::free_func free,
                              void* opaque)
{
    alloc_ = alloc;
    free_ = free;
    opaque_ = opaque;
    if (compress) {
        encoder_ = BrotliEncoderCreateInstance(alloc_, free_, opaque_);
        if (!encoder_)
            boost::throw_exception(std::bad_alloc());

        if (!BrotliEncoderSetParameter(static_cast<BrotliEncoderState *>(encoder_), BROTLI_PARAM_QUALITY, level_))
            boost::throw_exception(std::bad_alloc());
    } else {
        decoder_ = BrotliDecoderCreateInstance(alloc_, free_, opaque_);
        if (!decoder_)
            boost::throw_exception(std::bad_alloc());
    }
    eof_ = false;
}


} // End namespace detail.

//----------------------------------------------------------------------------//

} } // End namespaces iostreams, boost.
