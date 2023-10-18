//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a prefetch_descriptor (AIT specific).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of a prefetch_descriptor (AIT specific).
    //!
    //! This descriptor cannot be present in other tables than an AIT
    //! because its tag reuses an MPEG-defined one.
    //!
    //! @see ETSI TS 101 812, 10.8.3.2.
    //! @ingroup descriptor
    //!
    class TSDUCKDLL PrefetchDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Module entry.
        //!
        struct TSDUCKDLL Entry
        {
            // Public members
            UString label {};               //!< Label string.
            uint8_t prefetch_priority = 0;  //!< Prefetch priority, 1 to 100.

            //!
            //! Default constructor.
            //! @param [in] str Label.
            //! @param [in] pri Prefetch priority, 1 to 100.
            //!
            Entry(const UString& str = UString(), uint8_t pri = 1);
        };

        //!
        //! List of service entries.
        //!
        typedef std::list<Entry> EntryList;

        // PrefetchDescriptor public members:
        uint8_t   transport_protocol_label = 0;  //!< Transport protocol label.
        EntryList entries {};                   //!< The list of module entries.

        //!
        //! Default constructor.
        //!
        PrefetchDescriptor();

        //!
        //! Constructor from a binary descriptor.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        PrefetchDescriptor(DuckContext& duck, const Descriptor& bin);

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
