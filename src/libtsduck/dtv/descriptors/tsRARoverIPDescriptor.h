//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB_over_RNT_stream_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an DVB_over_RNT_stream_descriptor
    //! @see ETSI TS 102 323 clause 5.3.5.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL RNToverIPDescriptor: public AbstractDescriptor
    {
    public:

        Time                   first_valid_date {};
        Time                   last_valid_date {};
        uint8_t                weighting = 0;           //!< 6 bits;
        bool                   complete_flag = false;   //!<
        UString                url {};

        //!
        //! Default constructor.
        //!
        RNToverIPDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        RNToverIPDescriptor(DuckContext& duck, const Descriptor& bin);

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
