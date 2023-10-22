//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a quality_extension_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an MPEG-defined HEVC_hierarchy_extension_descriptor.
    //!
    //! Note that this descriptor only conveys the names of the metrics that are present. Actual
    //! timed mettic infomration is provided in the Quality_Access_Unt() - ISO/IEC 13818-1 clause 2.20
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.119 and ISO/ISC 23001-10
    //! @ingroup descriptor
    //!
    class TSDUCKDLL QualityExtensionDescriptor : public AbstractDescriptor
    {
    public:
        // QualityExtensionDescriptor public members:
        uint8_t               field_size_bytes = 0;  //!< constant size in bytes of the value for a metric in each sample
        std::vector<uint32_t> metric_codes {};       //!< metrics that are present in teh bitstream

        //!
        //! Default constructor.
        //!
        QualityExtensionDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        QualityExtensionDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual DID extendedTag() const override;
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
