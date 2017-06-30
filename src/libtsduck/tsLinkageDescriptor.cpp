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
//  Representation of a generic linkage_descriptor.
//  Specialized classes exist, depending on the linkage type.
//
//----------------------------------------------------------------------------

#include "tsLinkageDescriptor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor (uint16_t ts, uint16_t onetw, uint16_t service, uint8_t ltype) :
    AbstractDescriptor (DID_LINKAGE),
    ts_id (ts),
    onetw_id (onetw),
    service_id (service),
    linkage_type (ltype),
    private_data ()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::LinkageDescriptor::LinkageDescriptor (const Descriptor& desc) :
    AbstractDescriptor (DID_LINKAGE),
    ts_id (0),
    onetw_id (0),
    service_id (0),
    linkage_type (0),
    private_data ()
{
    deserialize (desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::serialize (Descriptor& desc) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    bbp->appendUInt16 (ts_id);
    bbp->appendUInt16 (onetw_id);
    bbp->appendUInt16 (service_id);
    bbp->appendUInt8 (linkage_type);
    bbp->append (private_data);

    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::LinkageDescriptor::deserialize (const Descriptor& desc)
{
    _is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() >= 7;
    private_data.clear();

    if (_is_valid) {
        const uint8_t* data = desc.payload();
        size_t size = desc.payloadSize();
        ts_id = GetUInt16 (data);
        onetw_id = GetUInt16 (data + 2);
        service_id = GetUInt16 (data + 4);
        linkage_type = data[6];
        private_data.copy (data + 7, size - 7);
    }
}
