//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBase64.h"
#include "tsMemory.h"

namespace {
    // Base64 alphabet.
    const ts::UChar alphabet[] = u"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Number of output Base64 characters (without padding) per input block size (up to BIN_BLOCK_SIZE bytes).
    const size_t b64_size[] = {0, 2, 3, 4};

    // Base64 reverse alphabet. Invalid characters are 0xFF.
    const uint8_t reverse_alphabet[128] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   62, 0xFF, 0xFF, 0xFF,   63,
          52,   53,   54,   55,   56,   57,   58,   59,   60,   61, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF,    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
          15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
          41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };

    // Number of output bytes per non-padding Base64 character (up to STR_BLOCK_SIZE).
    const size_t bin_size[] = {0, 0, 1, 2, 3};
}


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::Base64::Base64(size_t line_size) :
    _line_size(line_size)
{
}


//----------------------------------------------------------------------------
// Reset the encoder and decoder.
//----------------------------------------------------------------------------

void ts::Base64::reset()
{
    _line_count = 0;
    _encoding_size = 0;
    _decoding_size = 0;
}


//----------------------------------------------------------------------------
// Add a character to output Base64 string.
//----------------------------------------------------------------------------

void ts::Base64::encodeOne(UString& b64, UChar c)
{
    b64.push_back(c);
    if (_line_size > 0 && ++_line_count >= _line_size) {
        b64.push_back('\n');
        _line_count = 0;
    }
}


//----------------------------------------------------------------------------
// Encode one input block. Append to output string.
//----------------------------------------------------------------------------

void ts::Base64::encodeBlock(UString& b64, const uint8_t* data, size_t size)
{
    // Fill a modifiable contiguous input block with zero padding.
    std::array<uint8_t, BIN_BLOCK_SIZE> in {0, 0, 0};
    size = std::min(size, in.size());
    MemCopy(in.data(), data, size);

    // Encode input data: encode first byte, then shift buffer and loop.
    for (size_t i = 0; i < b64_size[size]; ++i) {
        encodeOne(b64, alphabet[in[0] >> 2]);
        in[0] = uint8_t(in[0] << 6) | (in[1] >> 2);
        in[1] = uint8_t(in[1] << 6) | (in[2] >> 2);
        in[2] = uint8_t(in[2] << 6);
    }

    // Add optional padding.
    if (size < BIN_BLOCK_SIZE) {
        for (size_t i = 0; i < STR_BLOCK_SIZE - b64_size[size]; ++i) {
            encodeOne(b64, PAD_CHAR);
        }
    }
}


//----------------------------------------------------------------------------
// Encode binary data and return partial Base64 string.
//----------------------------------------------------------------------------

void ts::Base64::encodeAdd(UString& b64, const void* vdata, size_t size)
{
    const uint8_t* data = reinterpret_cast<const uint8_t*>(vdata);

    // If previous input was buffered, fill it with additional data.
    if (_encoding_size > 0 && size > 0) {
        assert(_encoding_size <= _encoding.size());
        const size_t add_size = std::min(size, _encoding.size() - _encoding_size);
        MemCopy(_encoding.data() + _encoding_size, data, add_size);
        _encoding_size += add_size;
        data += add_size;
        size -= add_size;
    }

    // If previous input is a full block, encode it.
    if (_encoding_size == _encoding.size()) {
        encodeBlock(b64, _encoding.data(), _encoding_size);
        _encoding_size = 0;
    }

    // Process full blocks from input.
    while (size >= BIN_BLOCK_SIZE) {
        encodeBlock(b64, data, BIN_BLOCK_SIZE);
        data += BIN_BLOCK_SIZE;
        size -= BIN_BLOCK_SIZE;
    }

    // Buffer remaining input.
    if (size > 0) {
        MemCopy(_encoding.data(), data, size);
        _encoding_size = size;
    }
}


//----------------------------------------------------------------------------
// Terminate binary data encoding, generate potential end of Base64 stream.
//----------------------------------------------------------------------------

void ts::Base64::encodeTerminate(UString& b64)
{
    if (_encoding_size > 0) {
        encodeBlock(b64, _encoding.data(), _encoding_size);
        _encoding_size = 0;
    }
    if (_line_size > 0 && _line_count > 0) {
        b64.push_back('\n');
        _line_count = 0;
    }
}


//----------------------------------------------------------------------------
// Decode one Base64 block. Append to output data.
//----------------------------------------------------------------------------

bool ts::Base64::decodeBlock(ByteBlock& bin, const UChar* b64, size_t size)
{
    // We must have exactly one block.
    if (size != STR_BLOCK_SIZE) {
        return false;
    }

    // Number of non-padding characters.
    size_t char_count = STR_BLOCK_SIZE;
    while (char_count > 0 && b64[char_count - 1] == PAD_CHAR) {
        char_count--;
    }

    // Decode as 4 6-bit values.
    std::array<uint8_t, STR_BLOCK_SIZE> dec {0, 0, 0, 0};
    for (size_t i = 0; i < char_count; ++i) {
        const size_t v = b64[i];
        if (v >= sizeof(reverse_alphabet) || reverse_alphabet[v] == 0xFF) {
            return false;
        }
        dec[i] = reverse_alphabet[v];
    }

    // Convert to 3 8-bit values.
    const size_t out_size = bin_size[char_count];
    uint8_t* out = bin.enlarge(out_size);
    if (out_size > 0) {
        out[0] = uint8_t(dec[0] << 2) | (dec[1] >> 4);
        if (out_size > 1) {
            out[1] = uint8_t(dec[1] << 4) | (dec[2] >> 2);
            if (out_size > 2) {
                out[2] = uint8_t(dec[2] << 6) | dec[3];
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Decode Base64 string and return partial binary data.
//----------------------------------------------------------------------------

bool ts::Base64::decodeAdd(ByteBlock& bin, const UString& b64)
{
    bool status = true;
    size_t b64_index = 0;

    // If previous input was buffered, fill it with additional data.
    if (_decoding_size > 0) {
        while (_decoding_size < _decoding.size() && b64_index < b64.size()) {
            _decoding[_decoding_size++] = b64[b64_index++];
        }
    }

    // If previous input is a full block, decode it.
    if (_decoding_size == _decoding.size()) {
        status = decodeBlock(bin, _decoding.data(), _decoding_size);
        _decoding_size = 0;
    }

    // Process full blocks from input Base64 string.
    while (status && b64_index + STR_BLOCK_SIZE <= b64.size()) {
        status = decodeBlock(bin, &b64[b64_index], STR_BLOCK_SIZE);
        b64_index += STR_BLOCK_SIZE;
    }

    // Buffer remaining input.
    while (status && b64_index < b64.size()) {
        _decoding[_decoding_size++] = b64[b64_index++];
    }

    return status;
}


//----------------------------------------------------------------------------
// Terminate Base64 decoding.
//----------------------------------------------------------------------------

bool ts::Base64::decodeTerminate(ByteBlock& bin)
{
    // In Base64 string shall have a multiple of 4 bytes. Hence, the buffer shall be empty.
    const bool status = _decoding_size == 0;
    _decoding_size = 0;
    return status;
}


//----------------------------------------------------------------------------
// Bulk Base64 encoding.
//----------------------------------------------------------------------------

void ts::Base64::Encode(UString& b64, const void* data, size_t size, size_t line_size)
{
    Base64 enc(line_size);
    b64.clear();
    enc.encodeAdd(b64, data, size);
    enc.encodeTerminate(b64);
}

ts::UString ts::Base64::Encoded(const void* data, size_t size, size_t line_size)
{
    UString b64;
    Base64 enc(line_size);
    enc.encodeAdd(b64, data, size);
    enc.encodeTerminate(b64);
    return b64;
}


//----------------------------------------------------------------------------
// Bulk Base64 decoding.
//----------------------------------------------------------------------------

bool ts::Base64::Decode(ByteBlock& bin, const UString& b64)
{
    Base64 dec;
    bin.clear();
    return dec.decodeAdd(bin, b64) && dec.decodeTerminate(bin);
}

ts::ByteBlock ts::Base64::Decoded(const UString& b64)
{
    ByteBlock bin;
    Decode(bin, b64);
    return bin;
}
