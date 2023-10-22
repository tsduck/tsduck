//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB emergency_information_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB emergency_information_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.24
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EmergencyInformationDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Event entry.
        //!
        struct TSDUCKDLL Event
        {
            Event() = default;                      //!< Constructor.
            uint16_t              service_id = 0;   //!< Service id.
            bool                  started = false;  //!< True: event started, false: event ended.
            uint8_t               signal_level = 0; //!< Signal level (0 or 1).
            std::vector<uint16_t> area_codes {};    //!< List of area code, 12 bits each.
        };

        typedef std::list<Event> EventList;  //!< List of events.

        // EmergencyInformationDescriptor public members:
        EventList events {};  //!< List of events.

        //!
        //! Default constructor.
        //!
        EmergencyInformationDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EmergencyInformationDescriptor(DuckContext& duck, const Descriptor& bin);

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
