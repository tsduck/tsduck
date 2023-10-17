//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ECM_repetition_rate_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of an ECM_repetition_rate_descriptor.
    //! @see ETSI EN 301 192, 9.7.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ECMRepetitionRateDescriptor : public AbstractDescriptor
    {
    public:
        // Public members:
        uint16_t  CA_system_id = 0;         //!< Conditional access system id.
        uint16_t  ECM_repetition_rate = 0;  //!< ECM repetition rate in milliseconds.
        ByteBlock private_data {};          //!< CAS-dependent private data.

        //!
        //! Default constructor.
        //!
        ECMRepetitionRateDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ECMRepetitionRateDescriptor(DuckContext& duck, const Descriptor& bin);

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
