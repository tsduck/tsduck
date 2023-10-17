//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DVB stuffing_descriptor
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Representation of a DVB stuffing_descriptor.
    //! @see ETSI EN 300 468, 6.2.40.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DVBStuffingDescriptor : public AbstractDescriptor
    {
    public:
        // DVBStuffingDescriptor public members:
        ByteBlock stuffing {};   //!< Stuffing data.

        //!
        //! Default constructor.
        //!
        DVBStuffingDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DVBStuffingDescriptor(DuckContext& duck, const Descriptor& bin);

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

    //!
    //! Legacy name for a DVB stuffing_descriptor.
    //!
    typedef DVBStuffingDescriptor StuffingDescriptor;
}
