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
//!
//!  @file
//!  Representation of MPEG PSI/SI descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsByteBlock.h"
#include "tsSafePtr.h"
#include "tsMPEG.h"

namespace ts {
    //!
    //! Representation of a MPEG PSI/SI descriptors in binary format.
    //!
    class TSDUCKDLL Descriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        Descriptor() : _data(0) {}

        //!
        //! Copy constructor.
        //! @param [in] desc Another instance to copy.
        //! @param [in] mode The descriptors' data are either shared (ts::SHARE) between the
        //! two descriptors or duplicated (ts::COPY).
        //!
        Descriptor(const Descriptor& desc, CopyShare mode);

        //!
        //! Constructor from full binary content.
        //! The content is copied into the section if valid.
        //! @param [in] data Address of the descriptor data.
        //! @param [in] size Size in bytes of the descriptor data.
        //!
        Descriptor(const void* data, size_t size);

        //!
        //! Constructor from full binary content.
        //! The content is copied into the section if valid.
        //! @param [in] bb Descriptor binary data.
        //!
        Descriptor(const ByteBlock& bb);

        //!
        //! Constructor from full binary content.
        //! The content is copied into the section if valid.
        //! @param [in] bb Descriptor binary data.
        //! @param [in] mode The data are either shared (ts::SHARE) between the
        //! descriptor and @a bb or duplicated (ts::COPY).
        //!
        Descriptor(const ByteBlockPtr& bb, CopyShare mode);

        //!
        //! Assignment operator.
        //! The content is referenced, and thus shared between the two objects.
        //! @param [in] desc Another instance to copy.
        //! @return A reference to this object.
        //!
        Descriptor& operator=(const Descriptor& desc)
        {
            _data = desc._data;
            return *this;
        }

        //!
        //! Duplication.
        //! Similar to assignment but the content is duplicated.
        //! @param [in] desc Another instance to copy.
        //! @return A reference to this object.
        //!
        Descriptor& copy(const Descriptor& desc)
        {
            _data = new ByteBlock(*desc._data);
            return *this;
        }

        //!
        //! Check if a descriptor has valid content.
        //! @return True if a descriptor has valid content.
        //!
        bool isValid() const
        {
            return !_data.isNull();
        }

        //!
        //! Invalidate descriptor content.
        //!
        void invalidate()
        {
            _data.clear();
        }

        //!
        //! Get the descriptor tag.
        //! @return The descriptor tag or the reserved value 0 if the descriptor is invalid.
        //!
        DID tag() const
        {
            return _data.isNull() ? 0 : _data->at(0);
        }

        //!
        //! Access to the full binary content of the descriptor.
        //! @return Address of the full binary content of the descriptor.
        //!
        const uint8_t* content() const
        {
            return _data->data();
        }

        //!
        //! Size of the binary content of the descriptor.
        //! @return Size of the binary content of the descriptor.
        //!
        size_t size() const
        {
            return _data->size();
        }

        //!
        //! Access to the payload of the descriptor.
        //! @return Address of the payload of the descriptor.
        //!
        const uint8_t* payload() const
        {
            return _data->data() + 2;
        }

        //!
        //! Access to the payload of the descriptor.
        //! @return Address of the payload of the descriptor.
        //!
        uint8_t* payload()
        {
            return _data->data() + 2;
        }

        //!
        //! Size of the payload of the descriptor.
        //! @return Size in bytes of the payload of the descriptor.
        //!
        size_t payloadSize() const
        {
            return _data->size() - 2;
        }

        //!
        //! Replace the payload of the descriptor.
        //! The tag is unchanged, the size is adjusted.
        //! @param [in] data Address of the new payload data.
        //! @param [in] size Size in bytes of the new payload data.
        //!
        void replacePayload(const void* data, size_t size);

        //!
        //! Replace the payload of the descriptor.
        //! The tag is unchanged, the size is adjusted.
        //! @param [in] payload The new payload data.
        //!
        void replacePayload(const ByteBlock& payload)
        {
            replacePayload(payload.data(), payload.size());
        }

        //!
        //! Resize (truncate or extend) the payload of the descriptor.
        //! The tag is unchanged, the size is adjusted.
        //! If the payload is extended, new bytes are zeroes.
        //! @param [in] size New size in bytes of the payload.
        //!
        void resizePayload(size_t size);

        //!
        //! Comparison operator.
        //! @param [in] desc Another descriptor to compare.
        //! @return True is the two descriptors are identical.
        //!
        bool operator==(const Descriptor& desc) const;

        //!
        //! Comparison operator.
        //! @param [in] desc Another descriptor to compare.
        //! @return True is the two descriptors are different.
        //!
        bool operator!=(const Descriptor& desc) const
        {
            return !(*this == desc);
        }

        //!
        //! Display the descriptor on an output stream.
        //! The content of the descriptor is interpreted according to the descriptor id.
        //! @param [in,out] strm Output stream (text output).
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptor.
        //! This is optional. Used by some descriptors the interpretation of which may
        //! vary depending on the table that they are in.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //! @return A reference to @a strm.
        //!
        std::ostream& display(std::ostream& strm,
                              int indent = 0,
                              TID tid = TID_NULL,
                              PDS pds = 0) const;

        //!
        //! This static routine displays a list of descriptors from a memory area
        //! @param [in,out] strm Output stream (text output).
        //! @param [in] data Address of the descriptor list.
        //! @param [in] size Size in bytes of the descriptor list.
        //! @param [in] indent Indentation width.
        //! @param [in] tid Table id of table containing the descriptors.
        //! This is optional. Used by some descriptors the interpretation of which may
        //! vary depending on the table that they are in.
        //! @param [in] pds Private Data Specifier. Used to interpret private descriptors.
        //! @return A reference to @a strm.
        //!
        static std::ostream& Display(std::ostream& strm,
                                     const void* data,
                                     size_t size,
                                     int indent = 0,
                                     TID tid = TID_NULL,
                                     PDS pds = 0);

    private:
        Descriptor(const Descriptor&) = delete;

        // Private fields
        ByteBlockPtr _data; // full binary content of the descriptor
    };

    //!
    //! Safe pointer for Descriptor (not thread-safe)
    //!
    typedef SafePtr<Descriptor, NullMutex> DescriptorPtr;

    //!
    //! Vector of Descriptor pointers
    //! Use class DescriptorList for advanced features.
    //!
    typedef std::vector<DescriptorPtr> DescriptorPtrVector;
}


//!
//! Display operator for descriptors.
//! The content of the descriptor is interpreted according to the descriptor id.
//! @param [in,out] strm Output stream (text output).
//! @param [in] desc The descriptor to output.
//! @return A reference to @a strm.
//!
TSDUCKDLL inline std::ostream& operator<<(std::ostream& strm, const ts::Descriptor& desc)
{
    return desc.display(strm);
}
