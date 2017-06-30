//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Representation of a CA_identifier_descriptor
//
//----------------------------------------------------------------------------

#include "tsCAIdentifierDescriptor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::CAIdentifierDescriptor::CAIdentifierDescriptor() :
    AbstractDescriptor (DID_CA_ID),
    casids ()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::CAIdentifierDescriptor::CAIdentifierDescriptor (const Descriptor& desc) :
    AbstractDescriptor (DID_CA_ID),
    casids ()
{
    deserialize (desc);
}


//----------------------------------------------------------------------------
// Constructor using a variable-length argument list.
// Each argument is a CA_system_id. All arguments are int, not uint16_t,
// since integer literals are int by default.
// The end of the argument list must be marked by -1.
//----------------------------------------------------------------------------

ts::CAIdentifierDescriptor::CAIdentifierDescriptor (int casid, ...) :
    AbstractDescriptor (DID_CA_ID),
    casids ()
{
    _is_valid = true;
    if (casid >= 0) {
        casids.push_back (uint16_t (casid));
        va_list ap;
        va_start (ap, casid);
        while ((casid = va_arg (ap, int)) >= 0) {
            casids.push_back (uint16_t (casid));
        }
        va_end (ap);
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::serialize (Descriptor& desc) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    for (size_t n = 0; n < casids.size(); ++n) {
        bbp->appendUInt16 (casids[n]);
    }

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::CAIdentifierDescriptor::deserialize (const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 2 == 0;
    casids.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        while (size >= 2) {
            casids.push_back (GetUInt16 (data));
            data += 2;
            size -= 2;
        }
    }
}
