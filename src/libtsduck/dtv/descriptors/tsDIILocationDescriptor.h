//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a DII_location_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a DII_location_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 101 812, 10.8.3.3.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL DIILocationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Module entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            uint16_t DII_identification = 0;  //!< Identifier, 15 bits.
            uint16_t association_tag = 0;     //!< Tag.

            //!
            //! Default constructor.
            //! @param [in] id DII id.
            //! @param [in] tag Association tag.
            //!
            Entry(uint16_t id = 0, uint16_t tag = 0);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        //!
        //! Maximum number of entries to fit in 255 bytes.
        //!
        static constexpr size_t MAX_ENTRIES = 63;

        // DIILocationDescriptor public members:
        uint8_t   transport_protocol_label = 0;  //!< Transport protocol label.
        EntryList entries {};                    //!< The list of module entries.

        //!
        //! Default constructor.
        //!
        DIILocationDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        DIILocationDescriptor(DuckContext& duck, const Descriptor& bin);

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
