//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB STC_reference_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ISDB STC_reference_descriptor.
    //! @see ARIB STD-B10, Part 3, 5.2.5
    //! @ingroup descriptor
    //!
    class TSDUCKDLL STCReferenceDescriptor : public AbstractDescriptor
    {
    public:
        // STCReferenceDescriptor public members:
        uint8_t     STC_reference_mode = 0;   //!< 4 bits.
        bool        external_event = false;   //!< Presence of external event.
        uint16_t    external_event_id = 0;    //!< When external_event == true.
        uint16_t    external_service_id = 0;  //!< When external_event == true.
        uint16_t    external_network_id = 0;  //!< When external_event == true.
        uint64_t    NPT_reference = 0;        //!< 33 bits, when STC_reference_mode == 1.
        uint64_t    STC_reference = 0;        //!< 33 bits, when STC_reference_mode == 1 or 3 or 5.
        MilliSecond time_reference = 0;       //!< HH:MM:SS.mmm, when STC_reference_mode == 3 or 5.
        ByteBlock   reserved_data {};         //!< When STC_reference_mode not in 0,1,3,5.

        //!
        //! Default constructor.
        //!
        STCReferenceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        STCReferenceDescriptor(DuckContext& duck, const Descriptor& bin);

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
