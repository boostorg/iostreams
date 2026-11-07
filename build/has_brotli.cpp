// Copyright 2026 Tobias Stensbeck
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://boost.org/LICENSE_1_0.txt)

#include <brotli/decode.h>
#include <brotli/encode.h>

int main()
{
    // Reference one symbol from each of brotlidec and brotlienc. With static
    // archives this pulls in decode.o and encode.o, whose dependencies reach
    // brotlicommon, so the probe only links when all three libraries resolve.
    return BrotliDecoderVersion() == 0 || BrotliEncoderVersion() == 0;
}
