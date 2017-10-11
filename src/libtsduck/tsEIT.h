//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//!  Representation of an Event Information Table (EIT)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractLongTable.h"
#include "tsDescriptorList.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an Event Information Table (EIT)
    //!
    class TSDUCKDLL EIT : public AbstractLongTable
    {
    public:
        class Event;
        //!
        //! List of events, indexed by event_id.
        //!
        typedef std::map<uint16_t,Event> EventMap;

        // EIT public members:
        uint16_t service_id;     //!< Service_id.
        uint16_t ts_id;          //!< Transport stream_id.
        uint16_t onetw_id;       //!< Original network id.
        uint8_t  segment_last;   //!< Segment last section number.
        TID      last_table_id;  //!< Last table id.
        EventMap events;         //!< Map of event: key=event_id, value=event_description.

        //!
        //! Compute an EIT table id.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @param [in] is_pf True for EIT present/following, false for EIT schedule.
        //! @param [in] eits_index For EIT schedule, index from 0x00 to 0x0F, ignored for EIT present/following.
        //! @return The corresponding table id.
        //!
        static TID ComputeTableId(bool is_actual, bool is_pf, uint8_t eits_index = 0);

        //!
        //! Default constructor.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //! @param [in] is_pf True for EIT present/following, false for EIT schedule.
        //! @param [in] eits_index For EIT schedule, index from 0x00 to 0x0F, ignored for EIT present/following.
        //! @param [in] version Table version number.
        //! @param [in] is_current True if table is current, false if table is next.
        //! @param [in] service_id identifier.
        //! @param [in] ts_id Transport stream identifier.
        //! @param [in] onetw_id Original network id.
        //!
        EIT(bool     is_actual = true,
            bool     is_pf = true,
            uint8_t  eits_index = 0,
            uint8_t  version = 0,
            bool     is_current = true,
            uint16_t service_id = 0,
            uint16_t ts_id = 0,
            uint16_t onetw_id = 0);

        //!
        //! Constructor from a binary table.
        //! @param [in] table Binary table to deserialize.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //!
        EIT(const BinaryTable& table, const DVBCharset* charset = 0);

        //!
        //! Check if this is an "actual" EIT.
        //! @return True for EIT Actual TS, false for EIT Other TS.
        //!
        bool isActual() const;

        //!
        //! Set if this is an "actual" EIT.
        //! @param [in] is_actual True for EIT Actual TS, false for EIT Other TS.
        //!
        void setActual(bool is_actual);

        //!
        //! Check if this is an EIT present/following.
        //! @return True for EIT present/following, false for EIT schedume.
        //!
        bool isPresentFollowing() const
        {
            return _table_id == TID_EIT_PF_ACT || _table_id == TID_EIT_PF_OTH;
        }

        // Inherited methods
        virtual void serialize(BinaryTable& table, const DVBCharset* = 0) const override;
        virtual void deserialize(const BinaryTable& table, const DVBCharset* = 0) override;
        virtual XML::Element* toXML(XML&, XML::Element*) const override;
        virtual void fromXML(XML&, const XML::Element*) override;

        //!
        //! Description of an event.
        //!
        class TSDUCKDLL Event
        {
        public:
            // Public members
            Time           start_time;      //!< Event start_time.
            Second         duration;        //!< Event duration in seconds.
            uint8_t        running_status;  //!< Running status code.
            bool           CA_controlled;   //!< Controlled by a CA_system.
            DescriptorList descs;           //!< Descriptor list.

            //!
            //! Default constructor.
            //!
            Event();
        };

        //!
        //! A static method to display a section.
        //! @param [in,out] display Display engine.
        //! @param [in] section The section to display.
        //! @param [in] indent Indentation width.
        //!
        static void DisplaySection(TablesDisplay& display, const Section& section, int indent);

    private:
        // Add a new section to a table being serialized
        // Section number is incremented. Data and remain are reinitialized.
        void addSection(BinaryTable& table, int& section_number, uint8_t* payload, uint8_t*& data, size_t& remain) const;
    };
}
