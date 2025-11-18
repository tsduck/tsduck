//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFluteFile.h"


//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

ts::FluteFile::FluteFile(const IPAddress&       source,
                         const IPSocketAddress& destination,
                         uint64_t               tsi,
                         uint64_t               toi,
                         const UString&         name,
                         const ByteBlockPtr&    content) :
    _source(source),
    _destination(destination),
    _tsi(tsi),
    _toi(toi),
    _name(name),
    _content(content != nullptr ? content : std::make_shared<ByteBlock>())
{
}
