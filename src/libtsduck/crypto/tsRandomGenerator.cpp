//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRandomGenerator.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::RandomGenerator::~RandomGenerator()
{
}

//----------------------------------------------------------------------------
// Get random data in a byte block.
//----------------------------------------------------------------------------

bool ts::RandomGenerator::readByteBlock(ByteBlock& data, size_t size)
{
    data.resize(size);
    return read(data.data(), size);
}
