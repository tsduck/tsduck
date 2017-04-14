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

#pragma once
#include "tsDescriptor.h"

namespace ts {

    class Descriptor;
    class DescriptorList;

    class TSDUCKDLL AbstractDescriptor
    {
    public:
        // Check if the descriptor is valid
        bool isValid() const {return _is_valid;}

        // Invalidate the descriptor. Must be rebuilt.
        void invalidate() {_is_valid = false;}

        // Get the descriptor tag.
        DID tag() const {return _tag;}

        // This abstract method serializes a descriptor.
        // The content of the Descriptor is replaced with a binary
        // representation of this object.
        virtual void serialize (Descriptor&) const = 0;

        // This abstract method deserializes a binary descriptor.
        // This object represents the interpretation of the binary descriptor.
        virtual void deserialize (const Descriptor&) = 0;

        // Deserialize from a descriptor list.
        void deserialize (const DescriptorList&, size_t index);

        // Virtual destructor
        virtual ~AbstractDescriptor () {}

    protected:
        // The descriptor tag can be modified by subclasses only
        DID _tag;

        // It is the responsibility of the subclasses to set the valid flag
        bool _is_valid;

        // Protected constructor for subclasses
        AbstractDescriptor (DID tag) : _tag (tag), _is_valid (false) {}

    private:
        // Unreachable constructors and operators.
        AbstractDescriptor ();
    };

    // Safe pointer for AbstractDescriptor (not thread-safe)
    typedef SafePtr <AbstractDescriptor, NullMutex> AbstractDescriptorPtr;
}
