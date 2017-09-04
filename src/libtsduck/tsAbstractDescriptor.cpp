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
//  Abstract base class for MPEG PSI/SI descriptors
//
//----------------------------------------------------------------------------

#include "tsAbstractDescriptor.h"
#include "tsDescriptorList.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Protected constructor for subclasses.
//----------------------------------------------------------------------------

ts::AbstractDescriptor::AbstractDescriptor(DID tag, const char* xml_name, PDS pds) :
    _tag(tag),
    _xml_name(xml_name),
    _is_valid(false),
    _required_pds(pds)
{
}


//----------------------------------------------------------------------------
// Copy constructor and assignment.
// Not really needed but compilers may warn that pointer members require
// explicit copy.
//----------------------------------------------------------------------------

ts::AbstractDescriptor::AbstractDescriptor(const AbstractDescriptor& other) :
    _tag(other._tag),
    _xml_name(other._xml_name), // static storage
    _is_valid(other._is_valid),
    _required_pds(other._required_pds)
{
}

ts::AbstractDescriptor& ts::AbstractDescriptor::operator=(const AbstractDescriptor& other)
{
    if (&other != this) {
        _tag = other._tag;
        _xml_name = other._xml_name; // static storage
        _is_valid = other._is_valid;
        _required_pds = other._required_pds;
    }
    return *this;
}


//----------------------------------------------------------------------------
// Deserialize from a descriptor list.
//----------------------------------------------------------------------------

void ts::AbstractDescriptor::deserialize(const DescriptorList& dlist, size_t index)
{
    if (index > dlist.count()) {
        invalidate();
    }
    else {
        deserialize(*dlist[index]);
    }
}
