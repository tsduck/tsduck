//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB enhanced_AC-3_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DVB enhanced_AC-3_descriptor.
    //! @see ETSI EN 300 468, D.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DVBEnhancedAC3Descriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        std::optional<uint8_t> component_type {};     //!< See ETSI EN 300 468, D.5.
        std::optional<uint8_t> bsid {};               //!< See ETSI EN 300 468, D.5.
        std::optional<uint8_t> mainid {};             //!< See ETSI EN 300 468, D.5.
        std::optional<uint8_t> asvc {};               //!< See ETSI EN 300 468, D.5.
        bool                   mixinfoexists = false; //!< See ETSI EN 300 468, D.5.
        std::optional<uint8_t> substream1 {};         //!< See ETSI EN 300 468, D.5.
        std::optional<uint8_t> substream2 {};         //!< See ETSI EN 300 468, D.5.
        std::optional<uint8_t> substream3 {};         //!< See ETSI EN 300 468, D.5.
        ByteBlock              additional_info {};    //!< See ETSI EN 300 468, D.5.

        //!
        //! Default constructor.
        //!
        DVBEnhancedAC3Descriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBEnhancedAC3Descriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();
        virtual DescriptorDuplication duplicationMode() const override;
        virtual bool merge(const AbstractDescriptor& desc) override;

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };

    //!
    //! Legacy name for a DVB enhanced_AC-3_descriptor.
    //!
    typedef DVBEnhancedAC3Descriptor EnhancedAC3Descriptor;
}
