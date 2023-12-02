//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a RAR_over_IP_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an RAR_over_IP_descriptor
    //!
    //! This descriptor cannot be present in other tables than a RNT
    //! because its tag reuses a DVB-defined one.
    //!
    //! @see ETSI TS 102 323 clause 5.3.6.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL RARoverIPDescriptor: public AbstractDescriptor
    {
    public:

        Time    first_valid_date {};     //!< The first date when this CRID authority reference can be used.
        Time    last_valid_date {};      //!< The first date when this CRID authority reference cannot be used.
        uint8_t weighting = 0;           //!< 6 bits. A hint to the PDR as to the order to try multiple records for a single CRID authority from the same resolution provider.
        bool    complete_flag = false;   //!< This flag indicates if the referenced CRI data is complete
        UString url {};                  //!< The URL describing the location where CRIDs belonging to this CRID authority can be resolved.

        //!
        //! Default constructor.
        //!
        RARoverIPDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        RARoverIPDescriptor(DuckContext& duck, const Descriptor& bin);

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
