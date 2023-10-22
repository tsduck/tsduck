//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DSM-CC stream_mode_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {

    //!
    //! Representation of a DSM-CC stream_mode_descriptor.
    //! @see ISO/IEC 13818-6, 8.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL StreamModeDescriptor : public AbstractDescriptor
    {
    public:
        // StreamModeDescriptor public members:
        uint8_t stream_mode = 0;    //!< Stream mode, state machine.

        //!
        //! Default constructor.
        //! @param [in] mode Stream mode.
        //!
        StreamModeDescriptor(uint8_t mode = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        StreamModeDescriptor(DuckContext& duck, const Descriptor& bin);

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
