//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a time_slice_fec_identifier_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a time_slice_fec_identifier_descriptor.
    //! @see ETSI EN 301 192, 9.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL TimeSliceFECIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        bool      time_slicing;        //!< See ETSI EN 301 192, 9.5.
        uint8_t   mpe_fec;             //!< See ETSI EN 301 192, 9.5, 2 bits.
        uint8_t   frame_size;          //!< See ETSI EN 301 192, 9.5, 3 bits.
        uint8_t   max_burst_duration;  //!< See ETSI EN 301 192, 9.5.
        uint8_t   max_average_rate;    //!< See ETSI EN 301 192, 9.5, 4 bits.
        uint8_t   time_slice_fec_id;   //!< See ETSI EN 301 192, 9.5, 4 bits.
        ByteBlock id_selector_bytes;   //!< See ETSI EN 301 192, 9.5.

        //!
        //! Default constructor.
        //!
        TimeSliceFECIdentifierDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        TimeSliceFECIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

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
