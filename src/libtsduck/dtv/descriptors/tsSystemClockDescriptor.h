//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an system_clock_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an system_clock_descriptor.
    //! @see ISO/IEC 13818-1, ITU-T Rec. H.222.0, 2.6.20.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL SystemClockDescriptor : public AbstractDescriptor
    {
    public:
        // SystemClockDescriptor public members:
        bool    external_clock_reference = false;  //!< Has an external clock reference.
        uint8_t clock_accuracy_integer = 0;        //!< 6 bits, fractional frequency accuracy of the system clock in parts per million.
        uint8_t clock_accuracy_exponent = 0;       //!< 3 bits, exponent for @a clock_accuracy_integer.

        //!
        //! Default constructor.
        //!
        SystemClockDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        SystemClockDescriptor(DuckContext& duck, const Descriptor& bin);

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
