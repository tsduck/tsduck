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
//  Representation of an parental_rating_descriptor
//
//----------------------------------------------------------------------------

#include "tsParentalRatingDescriptor.h"



//----------------------------------------------------------------------------
// Default constructor:
//----------------------------------------------------------------------------

ts::ParentalRatingDescriptor::ParentalRatingDescriptor() :
    AbstractDescriptor (DID_PARENTAL_RATING),
    entries ()
{
    _is_valid = true;
}


//----------------------------------------------------------------------------
// Constructor from a binary descriptor
//----------------------------------------------------------------------------

ts::ParentalRatingDescriptor::ParentalRatingDescriptor (const Descriptor& desc) :
    AbstractDescriptor (DID_PARENTAL_RATING),
    entries ()
{
    deserialize (desc);
}


//----------------------------------------------------------------------------
// Constructor with one entry
//----------------------------------------------------------------------------

ts::ParentalRatingDescriptor::ParentalRatingDescriptor (const std::string& language_, uint8_t rating_) :
    AbstractDescriptor (DID_PARENTAL_RATING),
    entries ()
{
    _is_valid = true;
    entries.push_back (Entry (language_, rating_));
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::ParentalRatingDescriptor::serialize (Descriptor& desc) const
{
    ByteBlockPtr bbp (new ByteBlock (2));
    CheckNonNull (bbp.pointer());

    for (EntryList::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        if (it->language_code.length() != 3 || bbp->size() > 251) {
            desc.invalidate();
            return;
        }
        bbp->append (it->language_code);
        bbp->appendUInt8 (it->rating);
    }
    
    (*bbp)[0] = _tag;
    (*bbp)[1] = uint8_t(bbp->size() - 2);
    Descriptor d (bbp, SHARE);
    desc = d;
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::ParentalRatingDescriptor::deserialize (const Descriptor& desc)
{
    if (!(_is_valid = desc.isValid() && desc.tag() == _tag && desc.payloadSize() % 4 != 0)) {
        return;
    }

    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();
    entries.clear();

    while (size >= 4) {
        entries.push_back (Entry (std::string (reinterpret_cast <const char*> (data), 3), data[3]));
        data += 4; size -= 4;
    }
}
