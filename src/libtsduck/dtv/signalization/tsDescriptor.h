//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of MPEG PSI/SI descriptors
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsDataBlock.h"
#include "tsTablesPtr.h"
#include "tsEDID.h"
#include "tsDescriptorContext.h"
#include "tsPSIRepository.h"
#include "tsxml.h"

namespace ts {

    class DuckContext;

    //!
    //! Representation of a MPEG PSI/SI descriptors in binary format.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL Descriptor : public DataBlock<8, 8>
    {
        TS_DEFAULT_ASSIGMENTS(Descriptor);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        using SuperClass = DataBlock<8, 8>;

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
        Descriptor(const Descriptor& desc, ShareMode mode) : SuperClass(desc, mode) {}

        //!
        //! Move constructor.
        //! @param [in,out] desc Another instance to move.
        //!
        Descriptor(Descriptor&& desc) noexcept : SuperClass(std::move(desc)) {}

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
        Descriptor(const void* data, size_t size) : SuperClass(data, size) {}

        //!
        //! Constructor from full binary content.
        //! The content is copied into the section if valid.
        //! @param [in] bb Descriptor binary data.
        //!
        Descriptor(const ByteBlock& bb) : SuperClass(bb) {}

        //!
        //! Constructor from full binary content.
        //! @param [in] bb Descriptor binary data.
        //! @param [in] mode The data are either shared (ShareMode::SHARE) between the
        //! descriptor and @a bb or duplicated (ShareMode::COPY).
        //!
        Descriptor(const ByteBlockPtr& bb, ShareMode mode) : SuperClass(bb, mode) {}

        //!
        //! Virtual destructor.
        //!
        virtual ~Descriptor() override;

        //!
        //! Invalidate descriptor content.
        //!
        void invalidate() { clear(); }

        //!
        //! Get the descriptor tag.
        //! @return The descriptor tag or the reserved value 0 if the descriptor is invalid.
        //!
        DID tag() const { return size() < 1 ? 0 : *content(); }

        //!
        //! Get the extension descriptor id.
        //! @return The extension descriptor id. For MPEG or DVB extension descriptors, this is
        //! a combination of the descriptor tag and the extension tag. For other descriptors,
        //! this is the descriptor tag only.
        //!
        XDID xdid() const;

        //!
        //! Access to the payload of the descriptor.
        //! @return Address of the payload of the descriptor.
        //!
        const uint8_t* payload() const { return size() < 2 ? nullptr : content() + 2; }

        //!
        //! Access to the payload of the descriptor.
        //! @return Address of the payload of the descriptor.
        //!
        uint8_t* payload() { return size() < 2 ? nullptr : rwContent() + 2; }

        //!
        //! Size of the payload of the descriptor.
        //! @return Size in bytes of the payload of the descriptor.
        //!
        size_t payloadSize() const { const size_t s = size(); return s < 2 ? 0 : s - 2; }

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
        //! @param [in] other Another descriptor to compare.
        //! @return True is the two descriptors are identical.
        //!
        bool operator==(const Descriptor& other) const { return SuperClass::operator==(other); }

        //!
        //! Deserialize the descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] edid Extended descriptor id.
        //! @return A safe pointer to an instance of a concrete subclass of AbstractDescriptor
        //! representing this descriptor. Return the null pointer if the descriptor could not
        //! be deserialized.
        //!
        AbstractDescriptorPtr deserialize(DuckContext& duck, EDID edid) const;

        //!
        //! Deserialize the descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] context Context of the descriptor. Used to understand its identity.
        //! @return A safe pointer to an instance of a concrete subclass of AbstractDescriptor
        //! representing this descriptor. Return the null pointer if the descriptor could not
        //! be deserialized.
        //!
        AbstractDescriptorPtr deserialize(DuckContext& duck, DescriptorContext& context) const;

        //!
        //! This method converts a descriptor to XML.
        //! If the descriptor has a specialized implementation, generate a specialized
        //! XML structure. Otherwise, generate a \<generic_descriptor> node.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] parent The parent node for the XML descriptor.
        //! @param [in,out] context Context of the descriptor. Used to understand its identity.
        //! @param [in] force_generic Force a \<generic_descriptor> node even if the descriptor can be specialized.
        //! @return The new XML element or zero if the descriptor is not valid.
        //!
        xml::Element* toXML(DuckContext& duck, xml::Element* parent, DescriptorContext& context, bool force_generic = false) const;

        //!
        //! This method converts an XML node as a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] node The root of the XML descriptor.
        //! @param [in] tid Optional table id of the table containing the descriptor.
        //! @return True if the XML element name is a valid descriptor name, false otherwise.
        //! If the name is valid but the content is incorrect, true is returned and this object is invalidated.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* node, TID tid = TID_NULL);

        //!
        //! This method converts an XML node as a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [out] edid Extended descriptor id of the XML descriptor.
        //! @param [in] node The root of the XML descriptor.
        //! @param [in] tid Optional table id of the table containing the descriptor.
        //! @return True if the XML element name is a valid descriptor name, false otherwise.
        //! If the name is valid but the content is incorrect, true is returned and this object is invalidated.
        //!
        bool fromXML(DuckContext& duck, EDID& edid, const xml::Element* node, TID tid = TID_NULL);

    private:
        // Common code for deserialize().
        AbstractDescriptorPtr deserializeImpl(DuckContext& duck, PSIRepository::DescriptorFactory fac) const;

        // The default copy destructor is deleted, we need a ShareMode parameter.
        Descriptor(const Descriptor&) = delete;
    };
}
