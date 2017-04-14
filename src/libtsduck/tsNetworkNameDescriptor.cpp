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
//  Representation of a network_name_descriptor
//
//----------------------------------------------------------------------------

#include "tsNetworkNameDescriptor.h"



//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::NetworkNameDescriptor::NetworkNameDescriptor (const std::string& name_) :
    AbstractDescriptor (DID_NETWORK_NAME),
    name (name_)
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::NetworkNameDescriptor::NetworkNameDescriptor (const Descriptor& desc) :
    AbstractDescriptor (DID_NETWORK_NAME),
    name ()
{
    deserialize (desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::serialize (Descriptor& desc) const
{
    if (name.length() > 255) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(name.length());
    bbp->append (name);

    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::NetworkNameDescriptor::deserialize (const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag;

    if (_is_valid) {
        name.assign (reinterpret_cast <const char*> (desc.payload()), desc.payloadSize());
    }
    else {
        name.clear();
    }
}
