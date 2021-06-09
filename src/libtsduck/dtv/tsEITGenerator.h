//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//!  Perform various transformations on an EIT PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSectionFile.h"
#include "tsSectionDemux.h"
#include "tsPacketizer.h"
#include "tsServiceIdTriplet.h"
#include "tsTSPacket.h"
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! List of EIT sections repetition profiles.
    //! @ingroup mpeg
    //!
    //! The EIT sections shall be repeated according to the type of EIT and the type of network.
    //!
    //! The enumeration values are sorted in order of importance. For instance, it is more important
    //! to reliably broadcast EIT p/f actual than others, EIT p/f than schedule, etc.
    //!
    //! EIT schedule are divided into two periods:
    //! - The "prime" period extends over the next few days. The repetition rate of those EIT's
    //!   is typically longer than EIT present/following but still reasonably fast. The duration
    //!   in days of the prime period depends on the type of network.
    //! - The "later" period includes all events after the prime period. The repetition rate of
    //!   those EIT's is typically longer that in the prime period.
    //!
    //! Standard EIT repetition rates
    //! -----------------------------
    //!
    //! | %EIT section type        | Sat/cable | Terrestrial
    //! | ------------------------ | --------- | -----------
    //! | EIT p/f actual           | 2 sec     | 2 sec
    //! | EIT p/f other            | 10 sec    | 20 sec
    //! | EIT sched prime days     | 8 days    | 1 day
    //! | EIT sched actual (prime) | 10 sec    | 10 sec
    //! | EIT sched other (prime)  | 10 sec    | 60 sec
    //! | EIT sched actual (later) | 30 sec    | 30 sec
    //! | EIT sched other (later)  | 30 sec    | 300 sec
    //!
    enum class EITProfile {
        PF_ACTUAL          = 0,   //!< EIT present/following actual.
        PF_OTHER           = 1,   //!< EIT present/following other.
        SCHED_ACTUAL_PRIME = 2,   //!< EIT schedule actual in the "prime" period.
        SCHED_OTHER_PRIME  = 3,   //!< EIT schedule other in the "prime" period.
        SCHED_ACTUAL_LATER = 4,   //!< EIT schedule actual after the "prime" period.
        SCHED_OTHER_LATER  = 5,   //!< EIT schedule other after the "prime" period.
    };

    //!
    //! EIT sections repetition profile.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL EITRepetitionProfile
    {
    public:
        //!
        //! Number of EIT sections repetition profiles.
        //!
        static constexpr size_t PROFILE_COUNT = size_t(EITProfile::SCHED_OTHER_LATER) + 1;

        //!
        //! Duration in days of the "prime" period for EIT schedule.
        //! EIT schedule for events in the prime period (i.e. the next few days)
        //! are repeated more often than for later events.
        //!
        size_t prime_days;

        //!
        //! Cycle time in seconds of each EIT sections repetition profile.
        //! The array is indexed by EITProfile.
        //!
        size_t cycle_seconds[PROFILE_COUNT];

        //!
        //! Standard EIT repetition profile for satellite and cable networks.
        //! @see ETSI TS 101 211, section 4.1.4
        //!
        static const EITRepetitionProfile SatelliteCable;

        //!
        //! Standard EIT repetition profile for terrestrial networks.
        //! @see ETSI TS 101 211, section 4.1.4
        //!
        static const EITRepetitionProfile Terrestrial;
    };

    //!
    //! EIT generation options.
    //! The options can be specified as a byte mask.
    //!
    enum class EITOption {
        NONE   = 0x0000,   //!< Generate nothing.
        ACTUAL = 0x0001,   //!< Generate EIT actual.
        OTHER  = 0x0002,   //!< Generate EIT other.
        PF     = 0x0004,   //!< Generate EIT present/following.
        SCHED  = 0x0008,   //!< Generate EIT schedule.
        ALL    = 0x000F,   //!< Generate all EIT's.
        INPUT  = 0x0010,   //!< Use input EIT's as EPG data.
    };
}
TS_ENABLE_BITMASK_OPERATORS(ts::EITOption);

namespace ts {
    //!
    //! Generate and insert EIT sections based on an EPG content.
    //! @ingroup mpeg
    //!
    //! EPG database
    //! ------------
    //! The EPG database is entirely in memory. It is initially empty and emptied using
    //! EITGenerator::reset(). Events are loaded in EPG using EITGenerator::loadEvents()
    //! (various flavours exist).
    //!
    //! The EPG can be saved in a SectionFile object using EITGenerator::saveEITs().
    //! This object can later be saved in binary or XML format.
    //!
    //! The EPG content is filled with generic EIT sections, either in binary or XML form.
    //! The structure of these EIT sections, p/f or schedule, the number and order of events,
    //! do not matter. The events are individually extracted and will be stored and sorted
    //! independently. These events will be used to fill the generated EIT sections.
    //! In short, EIT section is just a convenient storage format for EPG events.
    //!
    //! Events can also be individually loaded, outside EIT sections but using the same binary
    //! format as in EIT, from field @a event_id to end of descriptor list.
    //!
    //! Principle of operation
    //! ----------------------
    //! The EITGenerator object is continuously invoked for all packets in a TS. Packets from
    //! the EIT PID or the stuffing PID are replaced by EIT packets, when necessary.
    //!
    //! It is important that the application passes all packets from the TS, not only the
    //! packets that the application wishes to replace. This is required so that the EITGenerator
    //! object can evaluate the bitrate and repetition rate of the generated EIT sections.
    //!
    //! EIT sections from the input EIT PID can also be used to populate the EPG database if
    //! the option EITOption::INPUT is set. This is a convenient way to generate EIT p/f in
    //! addition to (or in replacement to) an input stream of EIT schedule.
    //!
    //! Basic EIT generation rules
    //! --------------------------
    //! - Using a bit-mask of EITOption, the EITGenerator object can selectively generate
    //!   any combination of EIT p/f (present/following) and/or EIT schedule, EIT actual
    //!   and/or EIT other.
    //! - We call "subtable" the collection of sections with same table id and same table
    //!   id extension (the service id in the case of an EIT). This is a general MPEG/DVB
    //!   definition.
    //! - The EIT syntax and semantics are specified in ETSI EN 300 468, section 5.2.4.
    //!   The usage guidelines are specified in ETSI TS 101 211, section 4.1.4.
    //! - An EIT p/f, when present, must have two sections. The present event is in section 0.
    //!   The next event is in section 1. If there is no event, the EIT section must be present
    //!   but empty (no event).
    //! - EIT schedule are structured as follow:
    //!   - The structure is based on a "reference midnight", typically the current day at 00:00:00.
    //!   - The total duration of an EPG is 64 days maximum, over 16 subtables per service.
    //!   - Each subtable covers a duration of 4 days. The first subtable starts at the reference
    //!     midnight.
    //!   - Each subtable (4 days) is divided in 32 "segments" of 3 hours.
    //!   - A segment spans over 8 sections (sections 0-7, 8-15, 16-23, 24-31, etc.)
    //!   - In each subtable, all segments from 0 to the last used one shall be present. Unused
    //!     segments after the last used one are optional.
    //!   - In each segment, all sections from 0 to the last non-empty one shall be present. Unused
    //!     segments after the last used one are optional. Empty segment shall be represented with
    //!     one empty section. The last section in each segment is set in the EIT field
    //!     segment_last_section_number.
    //!   - The last EIT schedule table id is set in the field last_table_id in all EIT sections.
    //!   - As a consequence, an EIT schedule is not a complete table in MPEG terms since some
    //!     sections are intentionally missing.
    //! - EIT schedule are divided into two periods with potentially distinct repetition rates:
    //!   - The "prime" period extends over the next few days. The repetition rate of those EIT's
    //!     is typically longer than EIT present/following but still reasonably fast. The duration
    //!     in days of the prime period depends on the type of network.
    //!   - The "later" period includes all events after the prime period. The repetition rate of
    //!     those EIT's is typically longer that in the prime period.
    //!
    //! @see ETSI EN 300 468, 5.2.4
    //! @see ETSI TS 101 211, 4.1.4
    //!
    class TSDUCKDLL EITGenerator :
        private SectionHandlerInterface,
        private SectionProviderInterface
    {
        TS_NOBUILD_NOCOPY(EITGenerator);
    public:

        //!
        //! Constructor.
        //!
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this object.
        //! @param [in] pid The PID containing EIT's to insert.
        //! @param [in] options EIT generation options.
        //! @param [in] profile The EIT repetition profile.
        //!
        explicit EITGenerator(DuckContext& duck,
                              PID pid = PID_EIT,
                              EITOption options = EITOption::ALL | EITOption::INPUT,
                              const EITRepetitionProfile& profile = EITRepetitionProfile::SatelliteCable);

        //!
        //! Reset the EIT generator to default state.
        //! The EPG content is deleted. The TS id and current time are forgotten.
        //!
        void reset();

        //!
        //! Set new EIT generation options.
        //! If EIT generation is already started, existing EIT's are not affected.
        //! @param [in] options EIT generation options.
        //!
        void setOptions(EITOption options);

        //!
        //! Set a new EIT repetition profile.
        //! The new parameters may be taken into account at the end of the current cycles only.
        //! @param [in] profile The EIT repetition profile.
        //!
        void setProfile(const EITRepetitionProfile& profile) { _profile = profile; }

        //!
        //! Define the "actual" transport stream id for generated EIT's.
        //! When this method is called, all events for the specified TS are stored in
        //! "EIT actual". All other events are stored in "EIT other".
        //! By default, when no explicit TS id is set, the first PAT or EIT actual
        //! which is seen by processPacket() defines the actual TS id.
        //! @param [in] ts_id Actual transport stream id.
        //!
        void setTransportStreamId(uint16_t ts_id);

        //!
        //! Set the maximum bitrate of the EIT PID.
        //! If set to zero (the default), EIT's are injected according to their cycle
        //! time, within the limits of the input PID and the stuffing.
        //! The PID bitrate limitation is effective only if the transport stream bitrate
        //! is specified.
        //! @param [in] bitrate Maximum bitrate of the EIT PID.
        //! @see setTransportStreamBitRate()
        //!
        void setMaxBitRate(BitRate bitrate) { setBitRateField(&EITGenerator::_max_bitrate, bitrate); }

        //!
        //! Set the current time in the stream processing.
        //!
        //! By default, the current time is synchronized on each input TDT or TOT.
        //! Calling this method is useful only when there is no TDT/TOT or to set
        //! a time reference before the first TDT or TOT.
        //!
        //! @param [in] current_utc Current UTC time in the context of the stream.
        //!
        void setCurrentTime(Time current_utc);

        //!
        //! Get the current time in the stream processing.
        //!
        //! The current time starts from the last reference clock (TDT, TOT or setCurrentTime())
        //! and is maintained from the declared bitrate and number of processed packets.
        //! If there is no declared bitrate, the last reference time is returned.
        //!
        //! @return Current UTC time in the context of the stream.
        //!
        Time getCurrentTime();

        //!
        //! Set the current bitrate of the transport stream.
        //! The bitrate is used to update the current time, packet after packet,
        //! from the last reference clock (TDT, TOT or setCurrentTime()).
        //! @param [in] bitrate Current transport stream bitrate in bits per second.
        //! @see setCurrentTime()
        //!
        void setTransportStreamBitRate(BitRate bitrate) { setBitRateField(&EITGenerator::_ts_bitrate, bitrate); }

        //!
        //! Process one packet from the stream.
        //! @param [in,out] pkt A TS packet from the stream. If the packet belongs
        //! to the EIT PID or the null PID, it may be updated with new content.
        //!
        void processPacket(TSPacket& pkt);

        //!
        //! Load EPG data from binary events descriptions.
        //!
        //! If the current clock is defined, events which are older (already terminated) are ignored.
        //! If the clock is not yet defined, all events are stored and obsolete events will be discarded
        //! when the clock is defined for the first time.
        //!
        //! @param [in] service Service id triplet for all events in the binary data.
        //! @param [in] data Address of binary events data. Each event is described using
        //! the same format as in an EIT section, from the @a event_id field to the end of the
        //! descriptor list. Several events can be concatenated. All events are individually extracted.
        //! @param [in] size Size in bytes of the event binary data.
        //! @see ETSI EN 300 468
        //! @see setCurrentTime()
        //!
        void loadEvents(const ServiceIdTriplet& service, const uint8_t* data, size_t size);

        //!
        //! Load EPG data from an EIT section.
        //!
        //! All events are individually extracted.
        //! Events which are older than the current stream clock are ignored.
        //!
        //! If the transport stream id is known, event are stored as actual or other based on their
        //! service DVB triplet only, regardless of the EIT type (actual or other).
        //! If the TS id is not yet defined and the section is an EIT actual, then the TS id of the EIT
        //! becomes the TS id of the current transport stream.
        //! If the TS id is not yet defined, the section is ignored if this is an EIT Other.
        //!
        //! @param [in] section A section object. Non-EIT sections are ignored.
        //!
        void loadEvents(const Section& section);

        //!
        //! Load EPG data from a vector of EIT sections.
        //! @param [in] sections A vector of sections. Non-EIT sections are ignored.
        //! @see loadEvents(const Section&)
        //!
        void loadEvents(const SectionPtrVector& sections);

        //!
        //! Load EPG data from all EIT sections in a section file.
        //! @param [in] sections A section file object. Non-EIT sections are ignored.
        //! @see loadEvents(const Section&)
        //!
        void loadEvents(const SectionFile& sections) { loadEvents(sections.sections()); }

        //!
        //! Save all current EIT sections in a section file.
        //! If the current time is not set, the oldest event time in the EPG database is used.
        //! An EIT sections are regenerated when necessary.
        //! EIT p/f are saved first. Then EIT schedule.
        //! @param [in,out] sections A section file object into which all current EIT sections are saved.
        //!
        void saveEITs(SectionFile& sections);

    private:

        // -----------------------
        // Description of an event
        // -----------------------

        class Event
        {
            TS_NOBUILD_NOCOPY(Event);
        public:
            uint16_t  event_id;    // Event id.
            Time      start_time;  // Decoded event start time.
            Time      end_time;    // Decoded event end time.
            ByteBlock event_data;  // Binary event data, from event_id to end of descriptor loop.

            // Constructor based on EIT section payload. The data and size are updated after building the event.
            Event(const uint8_t*& data, size_t& size);
        };

        typedef SafePtr<Event> EventPtr;
        typedef std::list<EventPtr> EventList;

        // -----------------------------
        // Description of an EIT section
        // -----------------------------

        class ESection
        {
            TS_NOBUILD_NOCOPY(ESection);
        public:
            bool       obsolete;     // The section is obsolete, discard it when found in an injection list.
            bool       regenerate;   // Regenerate all EIT schedule in the segment (3 hours, up to 8 sections).
            bool       injected;     // Indicate that the data part of the section is used in a packetizer.
            Time       next_inject;  // Date of next injection.
            Time       start_time;   // First event start time.
            Time       end_time;     // Last event end time.
            SectionPtr section;      // Safe pointer to the EIT section.

            // Constructor, build an empty section for the specified service (CRC32 not set).
            ESection(const ServiceIdTriplet& service_id, TID tid, uint8_t section_number, uint8_t last_section_number);

            // Indicate that the section will be modified. It the section is or has recently been used in a
            // packetizer, a copy of the section is created first to avoid corrupting the section being packetized.
            void startModifying();

            // Toogle the actual/other status for the section.
            void toggleActual(bool actual);
        };

        typedef SafePtr<ESection> ESectionPtr;
        typedef std::list<ESectionPtr> ESectionList;

        // ------------------------------------------------------------------
        // Description of an EIT schedule segment (3 hours, up to 8 sections)
        // ------------------------------------------------------------------

        // When the list of events is changed in the segment, mark all sections as "regenerate".
        // Next time a section will be selected for injection, all sections in the segment will
        // be regenerated.

        class ESegment
        {
            TS_NOBUILD_NOCOPY(ESegment);
        public:
            const Time   start_time;      // Segment start time (a multiple of 3 hours). Never change.
            uint8_t      table_id;        // Current table id. May change after midnight.
            uint8_t      section_number;  // First section number in the segment. May change after midnight.
            EventList    events;          // List of events in the segment, sorted by start time.
            ESectionList sections;        // Current list of sections in the segment, sorted by start time.

            // Constructor. Build one empty section (all segments must have one empty section at least).
            ESegment(EITGenerator& eit_gen, const ServiceIdTriplet& service_id, const Time& seg_start_time);
        };

        typedef SafePtr<ESegment> ESegmentPtr;
        typedef std::list<ESegmentPtr> ESegmentList;

        // ------------------------
        // Description of a service
        // ------------------------

        class EService
        {
        public:
            ESectionPtr  present;     // EIT p/f section 0 ("present").
            ESectionPtr  following;   // EIT p/f section 1 ("following").
            ESegmentList segments;    // List of 3-hour segments (EPG events and EIT schedule sections).

            // Constructor.
            EService();
        };

        // -------------------
        // Event database root
        // -------------------

        // There are two main data roots, the event database and the injection lists.
        //
        // The event database is a map of EService, indexed by ServiceIdTriplet. This is
        // a static structure where new events are stored and obsolete events are removed.
        //
        // The injection lists are organized by repetition profile, in order of profile
        // priority (from EIT p/f actual to EID sched other/later). In each list, all
        // sections have the same profile and, consequently, the same repetition rate.
        // The sections are sorted in order of next injection. When a section is ready
        // to inject, it is passed to the packetizer and requeued at the end of the list
        // for the next injection.

        typedef std::map<ServiceIdTriplet, EService> EServiceMap;
        typedef std::array<ESectionList, EITRepetitionProfile::PROFILE_COUNT> ESectionListArray;

        // ---------------------------
        // EITGenerator private fields
        // ---------------------------

        DuckContext&         _duck;            // TSDuck execution context.
        const PID            _eit_pid;         // PID for input and generated EIT's.
        uint16_t             _ts_id;           // Transport stream id (to differentiate EIT actual and others).
        bool                 _ts_id_set;       // Boolean: value in _ts_id is valid.
        PacketCounter        _packet_index;    // Packet counter in the TS.
        BitRate              _max_bitrate;     // Max EIT bitrate.
        BitRate              _ts_bitrate;      // Declared TS bitrate.
        Time                 _next_midnight;   // Time after which all EIT schedule shall be reorganized.
        Time                 _ref_time;        // Last reference time.
        PacketCounter        _ref_time_pkt;    // Packet index at last reference time.
        PacketCounter        _eit_inter_pkt;   // Inter-packet distance in the EIT PID (zero if unbound).
        PacketCounter        _last_eit_pkt;    // Packet index at last EIT insertion.
        EITOption            _options;         // EIT generation options flags.
        EITRepetitionProfile _profile;         // EIT repetition profile.
        SectionDemux         _demux;           // Section demux for input stream, get PAT, TDT, TOT, EIT.
        Packetizer           _packetizer;      // Packetizer for generated EIT's.
        EServiceMap          _services;        // Map of services -> segments -> events and sections.
        ESectionListArray    _injects;         // Arrays of sections for injection.
        size_t               _obsolete_count;  // Number of obsolete sections in the injection lists.

        // Set a bitrate field and update EIT inter-packet.
        void setBitRateField(BitRate EITGenerator::* field, BitRate bitrate);

        // If the reference time is not set, force it to the start time of the oldest event in the database.
        void forceReferenceTime();

        // Update the EIT database according to the current time.
        // Obsolete events, sections and segments are discarded.
        // Segments which must be regenerated are marked as such (will be actually regenerated later, when used).
        // In case of "midnight effect" (moving to another day), reorganize all EIT schedule.
        void updateForNewTime(Time now);

        // Regenerate, if necessary, EIT p/f in a service.
        void regeneratePresentFollowing(const ServiceIdTriplet& service_id, const Time& now);
        void regeneratePresentFollowing(const ServiceIdTriplet& service_id, ESectionPtr& sec, TID tid, bool section_number, const EventPtr& event);

        // Regenerate all EIT schedule in a segment, create missing segments and sections.
        void regenerateSegment(const ServiceIdTriplet& service_id, Time now, Time segment_start_time);

        // Mark a section as obsolete, garbage collect obsolete sections if too many were not
        // naturally discarded from the injection lists. Also apply to entire segments.
        void markObsoleteSection(ESection& sec);
        void markObsoleteSegment(ESegment& seg);

        // Mark all sections in a segment as to be regenerated. This shall be invoked when
        // an event is updated in a segment. We don't want to regenerate all section each
        // time an event is updated (which usually happen in blasts). We simply mark the
        // sections for regeneration. They will be regenerated when the sections are needed.
        void markRegenerateSegment(ESegment& seg);

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;
    };
}
