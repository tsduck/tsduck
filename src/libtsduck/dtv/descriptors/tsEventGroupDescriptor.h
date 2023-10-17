//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB event_group_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Representation of an ISDB event_group_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.34
    //! @ingroup descriptor
    //!
    class TSDUCKDLL EventGroupDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Actual event entry.
        //!
        struct TSDUCKDLL ActualEvent
        {
            ActualEvent() = default;  //!< Constructor.
            uint16_t service_id = 0;  //!< Service id.
            uint16_t event_id = 0;    //!< Event id.
        };

        //!
        //! Actual network event entry.
        //!
        struct TSDUCKDLL OtherEvent
        {
            OtherEvent() = default;            //!< Constructor.
            uint16_t original_network_id = 0;  //!< Original network id.
            uint16_t transport_stream_id = 0;  //!< Transport stream id.
            uint16_t service_id = 0;           //!< Service id.
            uint16_t event_id = 0;             //!< Event id.
        };

        typedef std::list<ActualEvent> ActualEventList;  //!< List of actual events.
        typedef std::list<OtherEvent> OtherEventList;    //!< List of other events.

        // EventGroupDescriptor public members:
        uint8_t         group_type = 0;    //!< 4 bits, group type.
        ActualEventList actual_events {};  //!< List of actual events.
        OtherEventList  other_events {};   //!< List of other events, when group_type == 4 or 5.
        ByteBlock       private_data {};   //!< Private data for other group types.

        //!
        //! Default constructor.
        //!
        EventGroupDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        EventGroupDescriptor(DuckContext& duck, const Descriptor& bin);

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
