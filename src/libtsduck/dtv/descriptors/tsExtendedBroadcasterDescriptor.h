//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB extended_broadcaster_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"
#include "tsByteBlock.h"

namespace ts {

    //!
    //! Representation of an ISDB extended_broadcaster_descriptor.
    //! @see ARIB STD-B10, Part 2, 6.2.43
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ExtendedBroadcasterDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Broadcaster entry.
        //!
        struct TSDUCKDLL Broadcaster
        {
            uint16_t original_network_id = 0;  //!< Original network id.
            uint8_t  broadcaster_id = 0;       //!< Broadcaster id.
            //!
            //! Constructor.
            //! @param [in] onid Original network id.
            //! @param [in] bcid Broadcaster id.
            //!
            Broadcaster(uint16_t onid = 0, uint8_t bcid = 0);
        };

        //!
        //! List of broadcasters entries.
        //!
        typedef std::list<Broadcaster> BroadcasterList;

        // ExtendedBroadcasterDescriptor public members:
        uint8_t         broadcaster_type = 0;            //!< 4 bits, broadcaster type.
        uint16_t        terrestrial_broadcaster_id = 0;  //!< Broadcaster id (aka terrestrial_sound_broadcaster_id), when broadcaster_type == 0x01 or 0x02.
        ByteBlock       affiliation_ids {};              //!< List of 8-bit affiliation ids, when broadcaster_type == 0x01 or 0x02.
        BroadcasterList broadcasters {};                 //!< List of broadcasters, when broadcaster_type == 0x01 or 0x02.
        ByteBlock       private_data {};                 //!< Private data when broadcaster_type == 0x01 or 0x02, reserved_future_use otherwise.

        //!
        //! Default constructor.
        //!
        ExtendedBroadcasterDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ExtendedBroadcasterDescriptor(DuckContext& duck, const Descriptor& bin);

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
