//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of MPEG PSI/SI descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsTablesPtr.h"
#include "tsEDID.h"
#include "tsxml.h"

namespace ts {

    class DuckContext;

    //!
    //! Representation of a MPEG PSI/SI descriptors in binary format.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL Descriptor
    {
    public:
        //!
        //! Default constructor.
        //!
        Descriptor() = default;

        //!
        //! Copy constructor.
        //! @param [in] desc Another instance to copy.
        //! @param [in] mode The descriptors' data are either shared (ShareMode::SHARE) between the
        //! two descriptors or duplicated (ShareMode::COPY).
        //!
        Descriptor(const Descriptor& desc, ShareMode mode);

        //!
        //! Move constructor.
        //! @param [in,out] desc Another instance to move.
        //!
        Descriptor(Descriptor&& desc) noexcept;

        //!
        //! Constructor from tag and payload.
        //! The content is copied into the section if valid.
        //! @param [in] tag Descriptor tag.
        //! @param [in] data Address of the descriptor payload.
        //! @param [in] size Size in bytes of the descriptor payload.
        //!
        Descriptor(DID tag, const void* data, size_t size);

        //!
        //! Constructor from tag and payload.
        //! The content is copied into the section if valid.
        //! @param [in] tag Descriptor tag.
        //! @param [in] data Descriptor payload.
        //!
        Descriptor(DID tag, const ByteBlock& data);

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
        //! @param [in] mode The data are either shared (ShareMode::SHARE) between the
        //! descriptor and @a bb or duplicated (ShareMode::COPY).
        //!
        Descriptor(const ByteBlockPtr& bb, ShareMode mode);

        //!
        //! Assignment operator.
        //! The content is referenced, and thus shared between the two objects.
        //! @param [in] desc Another instance to copy.
        //! @return A reference to this object.
        //!
        Descriptor& operator=(const Descriptor& desc);

        //!
        //! Move assignment operator.
        //! @param [in,out] desc Another instance to move.
        //! @return A reference to this object.
        //!
        Descriptor& operator=(Descriptor&& desc) noexcept;

        //!
        //! Duplication.
        //! Similar to assignment but the content is duplicated.
        //! @param [in] desc Another instance to copy.
        //! @return A reference to this object.
        //!
        Descriptor& copy(const Descriptor& desc);

        //!
        //! Check if a descriptor has valid content.
        //! @return True if a descriptor has valid content.
        //!
        bool isValid() const { return !_data.isNull(); }

        //!
        //! Invalidate descriptor content.
        //!
        void invalidate() { _data.clear(); }

        //!
        //! Get the descriptor tag.
        //! @return The descriptor tag or the reserved value 0 if the descriptor is invalid.
        //!
        DID tag() const { return _data.isNull() ? 0 : _data->at(0); }

        //!
        //! Get the extended descriptor id.
        //! @param [in] pds Associated private data specifier.
        //! @param [in] tid Check if the descriptor is table-specific for this table-id.
        //! @return The extended descriptor id.
        //!
        EDID edid(PDS pds = 0, TID tid = TID_NULL) const;

        //!
        //! Get the extended descriptor id.
        //! @param [in] pds Associated private data specifier.
        //! @param [in] table Check if the descriptor is table-specific for this table.
        //! @return The extended descriptor id.
        //!
        EDID edid(PDS pds, const AbstractTable* table) const;

        //!
        //! Access to the full binary content of the descriptor.
        //! @return Address of the full binary content of the descriptor.
        //!
        const uint8_t* content() const { return _data->data(); }

        //!
        //! Size of the binary content of the descriptor.
        //! @return Size of the binary content of the descriptor.
        //!
        size_t size() const { return _data->size(); }

        //!
        //! Access to the payload of the descriptor.
        //! @return Address of the payload of the descriptor.
        //!
        const uint8_t* payload() const { return _data->data() + 2; }

        //!
        //! Access to the payload of the descriptor.
        //! @return Address of the payload of the descriptor.
        //!
        uint8_t* payload() { return _data->data() + 2; }

        //!
        //! Size of the payload of the descriptor.
        //! @return Size in bytes of the payload of the descriptor.
        //!
        size_t payloadSize() const { return _data->size() - 2; }

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
        void replacePayload(const ByteBlock& payload) { replacePayload(payload.data(), payload.size()); }

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
        TS_UNEQUAL_OPERATOR(Descriptor)

        //!
        //! Deserialize the descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] pds Associated private data specifier.
        //! @param [in] tid Optional table id of the table containing the descriptor.
        //! @return A safe pointer to an instance of a concrete subclass of AbstractDescriptor
        //! representing this descriptor. Return the null pointer if the descriptor could not
        //! be deserialized.
        //!
        AbstractDescriptorPtr deserialize(DuckContext& duck, PDS pds = 0, TID tid = TID_NULL) const;

        //!
        //! Deserialize the descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] pds Associated private data specifier.
        //! @param [in] table Table containing the descriptor (can be null).
        //! @return A safe pointer to an instance of a concrete subclass of AbstractDescriptor
        //! representing this descriptor. Return the null pointer if the descriptor could not
        //! be deserialized.
        //!
        AbstractDescriptorPtr deserialize(DuckContext& duck, PDS pds, const AbstractTable* table) const;

        //!
        //! This method converts a descriptor to XML.
        //! If the descriptor has a specialized implementation, generate a specialized
        //! XML structure. Otherwise, generate a \<generic_descriptor> node.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML descriptor.
        //! @param [in] pds Associated private data specifier.
        //! @param [in] tid Optional table id of the table containing the descriptor.
        //! @param [in] forceGeneric Force a \<generic_descriptor> node even if the descriptor can be specialized.
        //! @return The new XML element or zero if the descriptor is not valid.
        //!
        xml::Element* toXML(DuckContext& duck, xml::Element* parent, PDS pds = 0, TID tid = TID_NULL, bool forceGeneric = false) const;

        //!
        //! This method converts an XML node as a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] node The root of the XML descriptor.
        //! @param [in] tid Optional table id of the table containing the descriptor.
        //! @return True if the XML element name is a valid descriptor name, false otherwise.
        //! If the name is valid but the content is incorrect, true is returned and this object is invalidated.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* node, TID tid = TID_NULL);

    private:
        Descriptor(const Descriptor&) = delete;

        ByteBlockPtr _data {}; // full binary content of the descriptor
    };
}
