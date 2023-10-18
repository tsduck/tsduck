//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DSM-CC stream_event_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Representation of a DSM-CC stream_event_descriptor.
    //! @see ISO/IEC 13818-6, 8.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL StreamEventDescriptor : public AbstractDescriptor
    {
    public:
        // StreamEventDescriptor public members:
        uint16_t  event_id = 0;     //!< Event id.
        uint64_t  event_NPT = 0;    //!< 33 bits, event Normal Play Time (NPT).
        ByteBlock private_data {};  //!< Specific private data.

        //!
        //! Default constructor.
        //! @param [in] id Event id.
        //! @param [in] npt Event NPT.
        //!
        StreamEventDescriptor(uint16_t id = 0, uint64_t npt = 0);

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        StreamEventDescriptor(DuckContext& duck, const Descriptor& bin);

        //!
        //! Check if all bytes in private part are ASCII characters.
        //! @return True if all bytes in private part are ASCII characters.
        //!
        bool asciiPrivate() const;

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
