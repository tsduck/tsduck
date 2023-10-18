//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a PDC_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a PDC_descriptor.
    //! @see ETSI EN 300 468, 6.2.30.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL PDCDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint8_t pil_month = 0;    //!< Month number in programme_identification_label (PIL).
        uint8_t pil_day = 0;      //!< Day number in programme_identification_label (PIL).
        uint8_t pil_hours = 0;    //!< Hours in programme_identification_label (PIL).
        uint8_t pil_minutes = 0;  //!< Minutes in programme_identification_label (PIL).

        //!
        //! Default constructor.
        //!
        PDCDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        PDCDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void serializePayload(PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
