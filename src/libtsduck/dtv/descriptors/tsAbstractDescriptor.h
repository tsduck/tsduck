//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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
    //! What to do when a descriptor of same type is added twice in a list.
    //!
    enum class DescriptorDuplication {
        ADD_ALWAYS,  //!< Always add new descriptor, multiple occurrences of descriptor of same type is normal. This is the default.
        ADD_OTHER,   //!< Add new descriptor of same type if not the exact same content.
        REPLACE,     //!< Replace the old descriptor of same type with the new one.
        IGNORE,      //!< Ignore the new descriptor of same type.
        MERGE,       //!< Merge the new descriptor into the old one using a descriptor-specific method.
    };

    //!
    //! Abstract base class for MPEG PSI/SI descriptors.
    //! @ingroup libtsduck descriptor
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
        DID tag() const { return _edid.did(); }

        //!
        //! Get the extended descriptor id.
        //! @return The extended descriptor id.
        //!
        EDID edid() const { return _edid; }

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
        //! @return The extended descriptor tag or XDID_NULL if this is not an extended descriptor.
        //!
        DID extendedTag() const { return _edid.didExtension(); }

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

        // Inherited methods.
        virtual Standards definingStandards(Standards current_standards = Standards::NONE) const override;

    protected:
        //!
        //! Protected constructor for subclasses.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] xml_name Descriptor name, as used in XML structures.
        //! @param [in] xml_legacy_name Table or descriptor legacy XML name. Ignored if null pointer.
        //!
        AbstractDescriptor(EDID edid, const UChar* xml_name, const UChar* xml_legacy_name = nullptr);

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
        EDID _edid {};

        // Unreachable constructors and operators.
        AbstractDescriptor() = delete;
    };
}
