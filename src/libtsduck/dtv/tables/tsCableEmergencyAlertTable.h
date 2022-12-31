//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 18 Cable Emergency Alert Table.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsATSCMultipleString.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an SCTE 18 Cable Emergency Alert Table.
    //! Note that the so-called "sequence_number" in the SCTE 18 standard
    //! is in fact the "version" of the table, in MPEG parlance.
    //! @see ANSI/SCTE 18, section 5.
    //! @ingroup table
    //!
    class TSDUCKDLL CableEmergencyAlertTable : public AbstractLongTable
    {
    public:
        class Location;
        class Exception;

        // Public members:
        uint8_t              protocol_version;             //!< SCTE 18 protocol version, should be zero.
        uint16_t             EAS_event_ID;                 //!< EAS event id.
        UString              EAS_originator_code;          //!< Originator code, a 3-character code (usually one of "PEP", "WXR", "CIV", "EAS").
        UString              EAS_event_code;               //!< Event code, usually a 3-character code but not required.
        ATSCMultipleString   nature_of_activation_text;    //!< Event short description.
        uint8_t              alert_message_time_remaining; //!< Remaining number of seconds, 0 to 120.
        Time                 event_start_time;             //!< Event start time or Time::Epoch if immediate.
        uint16_t             event_duration;               //!< Event duration in minutes 0 or 15 to 6000.
        uint8_t              alert_priority;               //!< 4 bits, alert priority
        uint16_t             details_OOB_source_ID;        //!< 0 or id of channel carrying details.
        uint16_t             details_major_channel_number; //!< 0 or major number of channel carrying details.
        uint16_t             details_minor_channel_number; //!< 0 or minor number of channel carrying details.
        uint16_t             audio_OOB_source_ID;          //!< 0 or id of channel carrying audio.
        ATSCMultipleString   alert_text;                   //!< Alert text.
        std::list<Location>  locations;                    //!< List of location descriptions. Must be 1 to 31 elements.
        std::list<Exception> exceptions;                   //!< List of service exceptions. Up to 255 elements.
        DescriptorList       descs;                        //!< Descriptor list.

        //!
        //! Description of a geographical location for the alert.
        //!
        class TSDUCKDLL Location
        {
        public:
            uint8_t  state_code;          //!< State code.
            uint8_t  county_subdivision;  //!< 4 bits, geographical subdivision of county.
            uint16_t county_code;         //!< County code, 0 to 999.

            //!
            //! Constructor.
            //! @param [in] state State code.
            //! @param [in] sub Geographical subdivision of county.
            //! @param [in] county County code, 0 to 999.
            //!
            Location(uint8_t state = 0, uint8_t sub = 0, uint16_t county = 0);
        };

        //!
        //! Description of a service exception for the alert.
        //!
        class TSDUCKDLL Exception
        {
        public:
            bool     in_band;               //!< When true, use in-band major/minor id, when false use OOB source id.
            uint16_t major_channel_number;  //!< 10 bits, major channel id.
            uint16_t minor_channel_number;  //!< 10 bits, minor channel id.
            uint16_t OOB_source_ID;         //!< Id of out-of-band service.

            //!
            //! Constructor.
            //! @param [in] oob Id of out-of-band service.
            //!
            Exception(uint16_t oob = 0);

            //!
            //! Constructor for an in-band id.
            //! @param [in] major 10 bits, major channel id.
            //! @param [in] minor 10 bits, minor channel id.
            //!
            Exception(uint16_t major, uint16_t minor);
        };

        //!
        //! Default constructor.
        //! @param [in] sequence_number The EAS sequence number. Same as table version in MPEG parlance
        //!
        CableEmergencyAlertTable(uint8_t sequence_number = 0);

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        CableEmergencyAlertTable(const CableEmergencyAlertTable& other);

        //!
        //! Constructor from a binary table.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] table Binary table to deserialize.
        //!
        CableEmergencyAlertTable(DuckContext& duck, const BinaryTable& table);

        // Inherited methods
        virtual bool isPrivate() const override;
        virtual uint16_t tableIdExtension() const override;
        DeclareDisplaySection();

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual size_t maxPayloadSize() const override;
        virtual void serializePayload(BinaryTable&, PSIBuffer&) const override;
        virtual void deserializePayload(PSIBuffer&, const Section&) override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;
    };
}
