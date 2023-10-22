//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsHash.h"
#include "tsByteBlock.h"


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::Hash::~Hash()
{
}


//----------------------------------------------------------------------------
// Compute a hash in one operation
//----------------------------------------------------------------------------

bool ts::Hash::hash(const void* data, size_t data_size, void* hash, size_t hash_maxsize, size_t* hash_retsize)
{
    return init() && add(data, data_size) && getHash(hash, hash_maxsize, hash_retsize);
}

bool ts::Hash::hash(const void* data, size_t data_size, ByteBlock& result)
{
    result.resize(hashSize());
    size_t retsize = 0;
    const bool ok = hash(data, data_size, &result[0], result.size(), &retsize);
    result.resize(ok ? retsize : 0);
    return ok;
}
