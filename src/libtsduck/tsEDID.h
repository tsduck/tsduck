//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!
//!  @file
//!  "Extended Descriptor Id", a synthetic value for identifying descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"

namespace ts {
    //!
    //! Extended MPEG descriptor id.
    //!
    //! For convenience, it is sometimes useful to identify descriptors using
    //! an "extended DID", a combination of DID, PDS or descriptor tag
    //! extension.
    //!
    class TSDUCKDLL EDID
    {
    private:
        uint64_t _edid; // 32-bit: PDS/extension, 24-bit: unused, 8-bit: did
    public:
        //!
        //! Constructor.
        //! @param [in] did Descriptor tag.
        //! @param [in] ext Private data specifier when @a did >= 0x80.
        //! Descriptor tag extension when @a did == 0x7F.
        //! Ignored when @a did < 0x7F.
        //!
        explicit EDID(DID did = 0xFF, uint32_t ext = 0xFFFFFFFF) : _edid((uint64_t(ext) << 32) | (did & 0xFF)) {}

        //!
        //! Check if the extended descriptor id is valid.
        //! @return True if valid.
        //!
        bool isValid() const {return (_edid & 0xFF) != 0xFF;}

        //!
        //! Check if the descriptor is a private one.
        //! @return True if the descriptor is a private one.
        //!
        bool isPrivateDescriptor() const {return (_edid & 0xFF) > 0x7F;}

        //!
        //! Check if the descriptor is an extension descriptor.
        //! @return True if the descriptor is an extension descriptor.
        //!
        bool isExtensionDescriptor() const {return (_edid & 0xFF) == 0x7F;}

        //!
        //! Get the descriptor id (aka tag).
        //! @return The descriptor id.
        //!
        DID did() const {return DID(_edid & 0xFF);}

        //!
        //! Get the private data specifier.
        //! @return The private data specifier or 0xFFFFFFFF if this is not a private descriptor.
        //!
        PDS pds() const {return did() >= 0x80 ? PDS((_edid >> 32) & 0xFFFFFFFF) : PDS(0xFFFFFFFF);}

        //!
        //! Get the descriptor tag extension.
        //! @return The descriptor tag extension or 0xFF if this is not an extension descriptor.
        //!
        DID didExt() const {return did() == DID_EXTENSION ? DID((_edid >> 32) & 0xFF) : DID(0xFF);}

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object == @a e.
        //!
        bool operator ==(const EDID& e) const {return _edid == e._edid;}

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object != @a e.
        //!
        bool operator !=(const EDID& e) const {return _edid != e._edid;}

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object < @a e.
        //!
        bool operator <(const EDID& e) const {return _edid <  e._edid;}

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object <= @a e.
        //!
        bool operator <=(const EDID& e) const {return _edid <= e._edid;}

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object > @a e.
        //!
        bool operator >(const EDID& e) const {return _edid >  e._edid;}

        //!
        //! Comparison operator.
        //! @param [in] e Other instance to compare.
        //! @return True is this object >= @a e.
        //!
        bool operator >=(const EDID& e) const {return _edid >= e._edid;}
    };
}
