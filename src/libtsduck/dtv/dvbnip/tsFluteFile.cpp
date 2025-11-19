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
                         const UString&         type,
                         const ByteBlockPtr&    content) :
    _source(source),
    _destination(destination),
    _tsi(tsi),
    _toi(toi),
    _name(name),
    _type(type),
    _content(content != nullptr ? content : std::make_shared<ByteBlock>())
{
}


//----------------------------------------------------------------------------
// Get a character string version of the file, if it is a text file.
//----------------------------------------------------------------------------

ts::UString ts::FluteFile::toText() const
{
    return UString::FromUTF8(reinterpret_cast<const char*>(_content->data()), _content->size());
}
