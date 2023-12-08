//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an HEVC_timing_and_HRD_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an HEVC_timing_and_HRD_descriptor.
    //!
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.97.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL HEVCTimingAndHRDDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool                    hrd_management_valid = false;  //!< See ISO/IEC 13818-1.
        std::optional<uint8_t>  target_schedule_idx {};        //!< 5 bits, see ISO/IEC 13818-1.
        std::optional<uint32_t> N_90khz {};                    //!< See ISO/IEC 13818-1.
        std::optional<uint32_t> K_90khz {};                    //!< See ISO/IEC 13818-1.
        std::optional<uint32_t> num_units_in_tick {};          //!< See ISO/IEC 13818-1.

        //!
        //! Default constructor.
        //!
        HEVCTimingAndHRDDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        HEVCTimingAndHRDDescriptor(DuckContext& duck, const Descriptor& bin);

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
