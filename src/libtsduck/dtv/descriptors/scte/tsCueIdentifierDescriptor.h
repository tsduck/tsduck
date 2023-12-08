//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a cue_identifier_descriptor (SCTE 35).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsSCTE35.h"

namespace ts {
    //!
    //! Representation of a cue_identifier_descriptor (SCTE 35).
    //! @see ANSI/SCTE 35, 8.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL CueIdentifierDescriptor : public AbstractDescriptor
    {
    public:
        // CueIdentifierDescriptor public members:
        uint8_t cue_stream_type = CUE_ALL_COMMANDS;   //!< Type of cue messages in the PID.

        //!
        //! Definition of names for cue stream types.
        //!
        static const Enumeration CueStreamTypeNames;

        //!
        //! Default constructor.
        //! @param [in] type Allowed command types.
        //!
        CueIdentifierDescriptor(uint8_t type = CUE_ALL_COMMANDS);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        CueIdentifierDescriptor(DuckContext& duck, const Descriptor& bin);

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
