//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
