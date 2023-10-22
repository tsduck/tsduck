//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for MPEG PSI/SI descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSignalization.h"
#include "tsEDID.h"
#include "tsPSI.h"
#include "tsTablesPtr.h"

namespace ts {
    //!
    //! Abstract base class for MPEG PSI/SI descriptors.
    //! @ingroup descriptor
    //!
    //! A descriptor subclass shall override the following methods:
    //! - extendedTag() (for MPEG-defined and DVB-defined extension descriptors)
    //! - clearContent()
    //! - serializePayload()
    //! - deserializePayload()
    //! - buildXML()
    //! - analyzeXML()
    //!
    //! Important: With extension descriptors (MPEG or DVB), note the following:
    //! - extendedTag() must be overriden and must return the expected extended descriptor tag.
    //! - serializePayload() does not need to add the extended descriptor tag, it has
    //!   already been added in the buffer by the AbstractDescriptor.
    //! - deserializePayload() must not read the extended descriptor tag, it has already
    //!   been extracted from the buffer and verified by AbstractDescriptor.
    //! - The DisplayDescriptor() function is called without extended descriptor tag.
    //!   See ts::TablesDisplay::displayDescriptorData()
    //!
    class TSDUCKDLL AbstractDescriptor: public AbstractSignalization
    {
        TS_RULE_OF_FIVE(AbstractDescriptor, override);
    public:
        //!
        //! Get the descriptor tag.
        //! @return The descriptor tag.
        //!
        DID tag() const { return _tag; }

        //!
        //! Get the extended descriptor id.
        //! @param [in] tid Check if the descriptor is table-specific for this table-id.
        //! @return The extended descriptor id.
        //!
        EDID edid(TID tid = TID_NULL) const;

        //!
        //! Get the extended descriptor id.
        //! @param [in] table Check if the descriptor is table-specific for this table.
        //! @return The extended descriptor id.
        //!
        EDID edid(const AbstractTable* table) const;

        //!
        //! What to do when a descriptor of the same type is added twice in a descriptor list.
        //! The default action is DescriptorDuplication::ADD, meaning that descriptors are added to the list.
        //! Descriptor subclasses should override this method to define a new action.
        //! @return The descriptor duplication mode for this class of descriptors.
        //!
        virtual DescriptorDuplication duplicationMode() const;

        //!
        //! Merge the content of a descriptor into this object.
        //! This method implements the duplication mode DescriptorDuplication::MERGE and
        //! is specific to each descriptor subclass. By default, the merge fails.
        //! @param [in] desc The other descriptor to merge into this object. Usually,
        //! @a desc has the same subclass as this object, although this is not required.
        //! This is up to the implementation of the subclass to decide what to do.
        //! @return True if the merge succeeded, false if it failed.
        //!
        virtual bool merge(const AbstractDescriptor& desc);

        //!
        //! For MPEG-defined and DVB-defined extension descriptors, get the extended descriptor tag (first byte in payload).
        //! @return The extended descriptor tag or EDID_NULL if this is not an extended descriptor.
        //!
        virtual DID extendedTag() const;

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
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] bin A binary descriptor object. Its content is replaced with a binary representation of this descriptor.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool serialize(DuckContext& duck, Descriptor& bin) const;

        //!
        //! This method deserializes a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to interpret according to the descriptor subclass.
        //! In case of success, this object is replaced with the interpreted content of @a bin.
        //! In case of error, this object is invalidated.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool deserialize(DuckContext& duck, const Descriptor& bin);

        //!
        //! Deserialize a descriptor from a descriptor list.
        //! In case of success, this object is replaced with the interpreted content of the binary descriptor.
        //! In case of error, this object is invalidated.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] dlist A list of binary descriptors.
        //! @param [in] index Index of the descriptor to deserialize in @a dlist.
        //! @return True in case of success, false if the descriptor is invalid.
        //!
        bool deserialize(DuckContext& duck, const DescriptorList& dlist, size_t index);

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
        //! When serialize() is called, the output binary descriptor is cleared and serializePayload()
        //! is called. A subclass shall implement serializePayload().
        //!
        //! Important: With extension descriptors (MPEG or DVB), serializePayload() does not need to add
        //! the extended descriptor tag, it has already been added in the buffer by AbstractDescriptor::serialize().
        //!
        //! @param [in,out] buf Serialization buffer. The subclass shall write the descriptor payload into
        //! @a buf. If any kind of error is reported in the buffer, the serialization is considered as
        //! invalid and the binary descriptor is invalid. Such errors include write error, such as attempting
        //! to write more data than allowed in a binary descriptor or any user-generated error using
        //! ts::Buffer::setUserError(). For "extended descriptors", the buffer starts after the "extension tag"
        //! which was already written by the caller.
        //!
        virtual void serializePayload(PSIBuffer& buf) const = 0;

        //!
        //! Deserialize the payload of the descriptor.
        //!
        //! When deserialize() is called, this object is cleared and validated. Then, deserializePayload()
        //! is invoked. A subclass shall implement deserializePayload().
        //!
        //! Important: With extension descriptors (MPEG or DVB), deserializePayload() must not read the
        //! extended descriptor tag, it has already been extracted from the buffer and verified by
        //! AbstractDescriptor::deserialize().
        //!
        //! @param [in,out] buf Deserialization buffer. The subclass shall read the descriptor payload from
        //! @a buf. The end of read is the end of the binary payload. If any kind of error is reported in
        //! the buffer or if the payload is not completely read, the deserialization is considered as invalid.
        //! For "extended descriptors", the buffer starts after the "extension tag".
        //!
        virtual void deserializePayload(PSIBuffer& buf) = 0;

    private:
        DID _tag {DID_NULL};    // Descriptor tag.
        PDS _required_pds = 0;  // Required private data specifier.

        // Unreachable constructors and operators.
        AbstractDescriptor() = delete;
    };
}
