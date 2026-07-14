//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractStream.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::AbstractStream::~AbstractStream()
{
}


//----------------------------------------------------------------------------
// Default implementation of readStreamComplete() using readStream()
//----------------------------------------------------------------------------

bool ts::AbstractStream::readStreamComplete(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort)
{
    uint8_t* data = reinterpret_cast<uint8_t*>(addr);
    size_t insize = 0;
    ret_size = 0;

    while (max_size > 0) {
        if (!readStream(data, max_size, insize, abort) || insize == 0) {
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

bool ts::AbstractStream::readStreamChunks(void* addr, size_t max_size, size_t chunk_size, size_t& ret_size, const AbortInterface* abort)
{
    ret_size = 0;

    // Can read only an integral number of chunks.
    if (chunk_size > 0) {
        if (max_size < chunk_size) {
            _reporter.report().error(u"internal error, buffer (%'d bytes) is smaller than chunk size (%'d bytes)", max_size, chunk_size);
            return false;
        }
        max_size -= max_size % chunk_size;
    }

    // Initial read operation.
    bool success = readStream(addr, max_size, ret_size, abort);

    // Read end of chunk if ends in the middle of a chunk.
    if (success && chunk_size > 0 && ret_size % chunk_size != 0) {
        uint8_t* data = reinterpret_cast<uint8_t*>(addr);
        size_t insize = 0;
        success = readStreamComplete(data + ret_size, chunk_size - ret_size % chunk_size, insize, abort);
        ret_size += insize;
    }

    // At end of file, truncate to chunk size (drop trailing partial chunk if any).
    if (chunk_size > 0 && ret_size % chunk_size != 0 && endOfStream()) {
        ret_size -= ret_size % chunk_size;
    }

    return success;
}
