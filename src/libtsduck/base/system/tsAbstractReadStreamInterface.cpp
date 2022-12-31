//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsAbstractReadStreamInterface.h"


//----------------------------------------------------------------------------
// Destructor.
//----------------------------------------------------------------------------

ts::AbstractReadStreamInterface::~AbstractReadStreamInterface()
{
}


//----------------------------------------------------------------------------
// Default implementation of readStreamComplete() using readStreamPartial()
//----------------------------------------------------------------------------

bool ts::AbstractReadStreamInterface::readStreamComplete(void* addr, size_t max_size, size_t& ret_size, Report& report)
{
    uint8_t* data = reinterpret_cast<uint8_t*>(addr);
    size_t insize = 0;
    ret_size = 0;

    while (max_size > 0) {
        if (!readStreamPartial(data, max_size, insize, report) || insize == 0) {
            return ret_size > 0; // error or end of file only if nothing read at all
        }
        assert(insize <= max_size);
        ret_size += insize;
        data += insize;
        max_size -= insize;
    }
    return true;
}


//----------------------------------------------------------------------------
// Default implementation of readStreamComplete() using readStreamPartial()
//----------------------------------------------------------------------------

bool ts::AbstractReadStreamInterface::readStreamChunks(void* addr, size_t max_size, size_t chunk_size, size_t& ret_size, Report& report)
{
    ret_size = 0;

    // Can read only an integral number of chunks.
    if (chunk_size > 0) {
        if (max_size < chunk_size) {
            report.error(u"internal error, buffer (%'d bytes) is smaller than chunk size (%'d bytes)", {max_size, chunk_size});
            return false;
        }
        max_size -= max_size % chunk_size;
    }

    // Initial read operation.
    bool success = readStreamPartial(addr, max_size, ret_size, report);

    // Read end of chunk if ends in the middle of a chunk.
    if (success && chunk_size > 0 && ret_size % chunk_size != 0) {
        uint8_t* data = reinterpret_cast<uint8_t*>(addr);
        size_t insize = 0;
        success = readStreamComplete(data + ret_size, chunk_size - ret_size % chunk_size, insize, report);
        ret_size += insize;
    }

    // At end of file, truncate to chunk size (drop trailing partial chunk if any).
    if (chunk_size > 0 && ret_size % chunk_size != 0 && endOfStream()) {
        ret_size -= ret_size % chunk_size;
    }

    return success;
}
