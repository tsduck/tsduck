//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a T2MI_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a T2MI_descriptor.
    //! @see ETSI EN 300 468, 6.4.14.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL T2MIDescriptor : public AbstractDescriptor
    {
    public:
        // T2MIDescriptor public members:
        uint8_t   t2mi_stream_id = 0;                  //!< Identifier of T2-MI packets (3 bits).
        uint8_t   num_t2mi_streams_minus_one = 0;      //!< Total number (minus 1) of T2-MI streams required to generate the complete DVB-T2 signal.
        bool      pcr_iscr_common_clock_flag = false;  //!< Common clock source between PMT's PCR and ISCR (Input Stream Clock Reference).
        ByteBlock reserved {};                         //!< Reserved bytes.

        //!
        //! Default constructor.
        //!
        T2MIDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        T2MIDescriptor(DuckContext& duck, const Descriptor& bin);

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
