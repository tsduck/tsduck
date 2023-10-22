//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an SCTE 35 SpliceSchedule command.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractSignalization.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Representation of an SCTE 35 SpliceSchedule command.
    //!
    //! @see ANSI/SCTE 35, 9.3.2.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SpliceSchedule : public AbstractSignalization
    {
    public:
        //!
        //! A map of 32-bit UTC time values, indexed by 8-bit component tags.
        //! Used when the program is splice component by component, not as a whole.
        //!
        typedef std::map<uint8_t, uint32_t> UTCByComponent;

        //!
        //! A SpliceSchedule command is made of a list of events.
        //!
        class Event
        {
        public:
            Event() = default;                      //! Constructor.
            uint32_t       event_id = 0;            //!< Splice event id.
            bool           canceled = false;        //!< When true, event is canceled, other fields are ignored.
            bool           splice_out = false;      //!< When true, this is a "splice out" event, "splice in" otherwise.
            bool           program_splice = false;  //!< When true, all components are spliced.
            bool           use_duration = false;    //!< When true, the duration of the splice out / splice in sequence is given.
            uint32_t       program_utc = 0;         //!< UTC time value of the event (valid if !canceled && program_splice && !immediate).
            UTCByComponent components_utc {};       //!< UTC time value of the event by component (valid if !canceled && !program_splice && !immediate).
            uint64_t       duration_pts = 0;        //!< Duration of the splice out / splice in sequence (valid if !canceled && use_duration).
            bool           auto_return = false;     //!< When true, there won't be an explicit "splice in" event, use duration_pts (valid if !canceled && use_duration).
            uint16_t       program_id = 0;          //!< Unique program id.
            uint8_t        avail_num = 0;           //!< Identification for a specific avail within one program_id.
            uint8_t        avails_expected = 0;     //!< Expected number of individual avails within the current viewing event.
        };

        //!
        //! A list of splice Event structures.
        //!
        typedef std::list<Event> EventList;

        // Public members, derived from SCTE 35 standard.
        EventList events {};  //!< The events in the SpliceSchedule command.

        //!
        //! Default constructor.
        //!
        SpliceSchedule();

        //!
        //! Display the splice insert command.
        //! @param [in,out] display Display engine.
        //! @param [in] margin Left margin content.
        //!
        void display(TablesDisplay& display, const UString& margin) const;

        //!
        //! Deserialize a SpliceSchedule command from binary data.
        //! @param [in] data Address of data to deserialize.
        //! @param [in] size Size of data buffer, possibly larger than the SpliceSchedule command.
        //! @return Deserialized size, -1 on incorrect data.
        //!
        int deserialize(const uint8_t* data, size_t size);

        //!
        //! Serialize the SpliceSchedule command.
        //! @param [in,out] data The SpliceSchedule command is serialized at the end of this byte block.
        //!
        void serialize(ByteBlock& data) const;

        //!
        //! Convert a 32-bit SCTE 35 @e utc_splice_time to an actual UTC time.
        //! @param [in] duck TSDuck execution context.
        //! @param [in] value A 32-bit SCTE 35 @e utc_splice_time.
        //! @return The corresponding UTC time.
        //!
        static Time ToUTCTime(const DuckContext& duck, uint32_t value);

        //!
        //! Convert a UTC time into a 32-bit SCTE 35 @e utc_splice_time.
        //! @param [in] duck TSDuck execution context.
        //! @param [in] value A UTC time.
        //! @return The corresponding 32-bit SCTE 35 @e utc_splice_time.
        //!
        static uint32_t FromUTCTime(const DuckContext& duck, const Time& value);

    protected:
        // Inherited methods
        virtual void clearContent() override;
        virtual void buildXML(DuckContext&, xml::Element*) const override;
        virtual bool analyzeXML(DuckContext&, const xml::Element*) override;

    private:
        // Full dump of utc_splice_time.
        static UString DumpSpliceTime(const DuckContext& duck, uint32_t value);

        // Dual interpretation of utc_splice_time XML attributes.
        static bool GetSpliceTime(const DuckContext& duck, const xml::Element* elem, const UString& attribute, uint32_t& value);
    };
}
