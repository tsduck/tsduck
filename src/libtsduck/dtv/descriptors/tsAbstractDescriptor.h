//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  Abstract base class for MPEG PSI/SI descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSignalization.h"
#include "tsTablesPtr.h"
#include "tsByteBlock.h"
#include "tsMPEG.h"

namespace ts {
    //!
    //! Abstract base class for MPEG PSI/SI descriptors.
    //! @ingroup descriptor
    //!
    //! A descriptor subclass shall override the following methods:
    //! - clearContent()
    //! - serializePayload()
    //! - deserializePayload()
    //! - buildXML()
    //! - analyzeXML()
    //!
    class TSDUCKDLL AbstractDescriptor: public AbstractSignalization
    {
    public:
        //!
        //! Get the descriptor tag.
        //! @return The descriptor tag.
        //!
        DID tag() const { return _tag; }

        //!
        //! Get the required private data specifier.
        //! @return The private data specifier which is required to interpret correctly
        //! this descriptor in a section. Return zero if this descriptor is a DVB-defined
        //! or MPEG-defined descriptor, not a private specifier.
        //!
        PDS requiredPDS() const { return _required_pds; }

        //!
        //! Check if this descriptor is a private descriptor.
        //! @return True if this descriptor is a private descriptor,
        //! false if it is a DVB-defined or MPEG-defined descriptor.
        //!
        bool isPrivateDescriptor() const { return _required_pds != 0; }

        //!
        //! This method serializes a descriptor.
        //!
        //! The subclass shall preferably override serializePayload(). As legacy, the subclass may directly override
        //! serialize() but this is not recommended for new descriptors. At some point, if we can refactor all
        //! descriptors to the new scheme using serializePayload() (which seems unlikely), serialize() will
        //! become "final" and will no longer allow override.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] bin A binary descriptor object. Its content is replaced with a binary representation of this descriptor.
        //!
        virtual void serialize(DuckContext& duck, Descriptor& bin) const;

        //!
        //! This method deserializes a binary descriptor.
        //!
        //! The subclass shall preferably override deserializePayload(). As legacy, the subclass may directly override
        //! deserialize() but this is not recommended for new descriptors. At some point, if we can refactor all
        //! descriptors to the new scheme using deserializePayload() (which seems unlikely), deserialize() will
        //! become "final" and will no longer allow override.
        //!
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to interpret according to the descriptor subclass.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //!
        virtual void deserialize(DuckContext& duck, const Descriptor& bin);

        //!
        //! Deserialize a descriptor from a descriptor list.
        //! In case of success, this object is replaced with the interpreted content of the binary descriptor.
        //! In case of error, this object is invalidated.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] dlist A list of binary descriptors.
        //! @param [in] index Index of the descriptor to deserialize in @a dlist.
        //!
        void deserialize(DuckContext& duck, const DescriptorList& dlist, size_t index);

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractDescriptor();

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] tag Descriptor tag.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] standards A bit mask of standards which define this structure.
        //! @param [in] pds Required private data specifier if this is a private descriptor.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractDescriptor(DID tag, const UChar* xml_name, Standards standards, PDS pds, const UChar* xml_legacy_name = nullptr);

        //!
        //! Serialize the payload of the descriptor.
        //!
        //! This is now the preferred method for descriptor serialization: use the default implementation
        //! of serialize() and let it call the overridden serializePayload().
        //!
        //! The default implementation generates an error. So, if a subclass overrides neither serialize()
        //! not serializePayload(), all serialization will fail.
        //!
        //! @param [in,out] buf Serialization buffer. The subclass shall write the descriptor payload into
        //! @a buf. If any kind of error is reported in the buffer, the serialization is considered as
        //! invalid and the binary descriptor is invalid. Such errors include write error, such as attempting
        //! to write more data than allowed in a binary descriptor or any user-generated error using
        //! ts::Buffer::setUserError().
        //!
        virtual void serializePayload(PSIBuffer& buf) const;

        //!
        //! Deserialize the payload of the descriptor.
        //!
        //! This is now the preferred method for descriptor deserialization: use the default implementation
        //! of deserialize() and let it call the overridden deserializePayload().
        //!
        //! The default implementation generates an error. So, if a subclass overrides neither deserialize()
        //! nor deserializePayload(), all deserialization will fail.
        //!
        //! @param [in,out] buf Deserialization buffer. The subclass shall read the descriptor payload from
        //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
        //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
        //!
        virtual void deserializePayload(PSIBuffer& buf);

        //!
        //! Tool for serialization: get a byte buffer for serialization.
        //! Legacy warning: This method is useful only when serialize() is directly overridden instead
        //! of serializePayload(). This is consquently considered as a legacy feature.
        //! @return A safe pointer to a two-byte byffer containing the descriptor tag and zero as length.
        //! @see serializeEnd()
        //!
        ByteBlockPtr serializeStart() const;

        //!
        //! Tool for serialization: complete a serialization.
        //! Legacy warning: This method is useful only when serialize() is directly overridden instead
        //! of serializeContent(). This is consquently considered as a legacy feature.
        //! @param [out] bin A binary descriptor object which receives the serialized object.
        //! @param [in] bbp Safe pointer containing the serialized data, typically returned by serializeStart().
        //! The tag and length will be updated.
        //! @return True if the serialized descriptor is valid.
        //! @see serializeStart()
        //!
        bool serializeEnd(Descriptor& bin, const ByteBlockPtr& bbp) const;

    private:
        DID _tag;           // Descriptor tag.
        PDS _required_pds;  // Required private data specifier.

        // Unreachable constructors and operators.
        AbstractDescriptor() = delete;
    };
}
