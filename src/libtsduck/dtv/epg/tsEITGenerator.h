//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generate and insert EIT sections based on an EPG content.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEITOptions.h"
#include "tsEITRepetitionProfile.h"
#include "tsSectionFile.h"
#include "tsSectionDemux.h"
#include "tsPacketizer.h"
#include "tsServiceIdTriplet.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Generate and insert EIT sections based on an EPG content.
    //! @ingroup mpeg
    //!
    //! To generate EIT sections, an instance of EITGenerator needs:
    //! - The identity of the actual TS. Set using EITGenerator::setTransportStreamId() or
    //!   using EITGenerator::processPacket() after a PAT is found.
    //! - The current UTC time. Set using EITGenerator::setCurrentTime() or using
    //!   EITGenerator::processPacket() after a TDT or TOT is found.
    //! - Events in the EPG database. Loaded using EITGenerator::loadEvents() or using
    //!   EITGenerator::processPacket() from incoming EIT's (if EITOption::INPUT is
    //!   selected in the generation options).
    //!
    //! The current time is automatically updated packet after packet, based on the last
    //! UTC time, the number of packets since then and the transport stream bitrate (shall
    //! be set using EITGenerator::setTransportStreamBitRate()).
    //!
    //! Generated EIT section can be saved in a SectionFile object (see EITGenerator::saveEITs())
    //! or inserted in the transport stream using EITGenerator::processPacket(). In the latter
    //! case, input null packets and EIT packets are replaced with packets for the generated
    //! EIT's (or null packets when necessary).
    //!
    //! EIT packet insertion is performed depending on cycle time of the various EIT sections
    //! (see EITGenerator::setProfile()). The maximum EIT bandwidth can be limited using
    //! EITGenerator::setMaxBitRate().
    //!
    //! EPG database
    //! ------------
    //! The EPG database is entirely in memory. It is initially empty and emptied using
    //! EITGenerator::reset(). Events are loaded in EPG using EITGenerator::loadEvents()
    //! (various flavours exist).
    //!
    //! The EPG can be saved in a SectionFile object using EITGenerator::saveEITs().
    //! This object can later be saved in binary or XML format. It can be reloaded later
    //! in another instance of EITGenerator.
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
    //! Implementation notes
    //! --------------------
    //! The number and content of the EIT sections depend on the EPG data and the time.
    //!
    //! The public methods which may trigger EIT sections modifications are:
    //! - loadEvents() : new events in the EPG.
    //! - setCurrentTime() : obsoletes events, creates / removes segments.
    //! - processPacket() : increase time, load events from input EIT's.
    //! - setOptions() : may change the types of EIT to generate.
    //! - setTransportStreamId() : toggles EIT actual or some EIT other.
    //! - setProfile() : change the repetition rates, ignored, used only when a section is requeued.
    //!
    //! The EIT regeneration takes time, so we need to limit the operations.
    //! - Changing the options, the transport stream id and the repetition profile is not supposed to
    //!   happen more than once and the processing time is not important.
    //! - Setting the time to a completely new reference is not frequent either.
    //! - Setting the time to a small increment is extremely frequent (each packet in fact). Impact:
    //!   - Update EIT p/f on some services.
    //!   - Remove a segment and associated EIT schedule sections when crossing a 3-hour segment.
    //!     We do not remove obsolete events in EIT schedule sections inside the current segment
    //!     unless the segment is forced to be regenerated.
    //!   - Midnight effect: When crossing 00:00:00, all EIT schedule sections must change.
    //!     Affected fields: table_id, section_number, last_section_number, segment_last_section_number,
    //!     last_table_id. These are only a few fields to patch "in place" (and recompute the CRC).
    //! - Loading events is usually performed in batch (load files) or the same events are loaded
    //!   again and again (input EIT sections). We use the following method:
    //!   - Existing events with same content are silently ignored.
    //!   - When a new event is loaded, the "regenerate" flag is set 1) globally, 2) on the service
    //!     and 3) on the 3-hour segment.
    //!   - When an EIT section needs to be injected, we check the global "regenerate" flag. When
    //!     set, all services and segments are inspected and regenerated when necessary. All "regenerate"
    //!     flags are then cleared.
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
                              EITOptions options = EITOptions::GEN_ALL | EITOptions::LOAD_INPUT,
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
        void setOptions(EITOptions options);

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
        //! Get the "actual" transport stream id for generated EIT's.
        //! @return Actual transport stream id or 0xFFFF if unset.
        //!
        uint16_t getTransportStreamId() { return _actual_ts_id_set ? _actual_ts_id : 0xFFFF; }

        //!
        //! Set the maximum bitrate of the EIT PID.
        //! If set to zero (the default), EIT's are injected according to their cycle
        //! time, within the limits of the input PID and the stuffing.
        //! The PID bitrate limitation is effective only if the transport stream bitrate
        //! is specified.
        //! @param [in] bitrate Maximum bitrate of the EIT PID.
        //! @see setTransportStreamBitRate()
        //!
        void setMaxBitRate(const BitRate& bitrate) { setBitRateField(&EITGenerator::_max_bitrate, bitrate); }

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
        //! @return Current UTC time in the context of the stream or Time::Epoch if the current time is not set.
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
        //! @return True in case of success, false on error.
        //! @see ETSI EN 300 468
        //! @see setCurrentTime()
        //!
        bool loadEvents(const ServiceIdTriplet& service, const uint8_t* data, size_t size);

        //!
        //! Load EPG data from an EIT section.
        //! @param [in] section A section object. Non-EIT sections are ignored.
        //! @param [in] get_actual_ts If true and the actual transport stream id is not yet defined
        //! and the section is an EIT actual, set the actual TS.
        //! @return True in case of success, false on error.
        //!
        bool loadEvents(const Section& section, bool get_actual_ts = false);

        //!
        //! Load EPG data from a vector of EIT sections.
        //! @param [in] sections A vector of sections. Non-EIT sections are ignored.
        //! @return True in case of success, false on error.
        //! @param [in] get_actual_ts If true and the actual transport stream id is not yet defined
        //! use the first EIT actual section to set the actual TS.
        //! @see loadEvents(const Section&)
        //!
        bool loadEvents(const SectionPtrVector& sections, bool get_actual_ts = false);

        //!
        //! Load EPG data from all EIT sections in a section file.
        //! @param [in] sections A section file object. Non-EIT sections are ignored.
        //! @param [in] get_actual_ts If true and the actual transport stream id is not yet defined
        //! use the first EIT actual section to set the actual TS.
        //! @return True in case of success, false on error.
        //! @see loadEvents(const Section&)
        //!
        bool loadEvents(const SectionFile& sections, bool get_actual_ts = false) { return loadEvents(sections.sections(), get_actual_ts); }

        //!
        //! Save all current EIT sections.
        //! If the current time is not set, the oldest event time in the EPG database is used.
        //! An EIT sections are regenerated when necessary.
        //! EIT p/f are saved first. Then EIT schedule.
        //! @param [in,out] sections A vector of safe pointers to sections into which all current EIT sections are saved.
        //!
        void saveEITs(SectionPtrVector& sections);

        //!
        //! Save all current EIT sections in a section file.
        //! If the current time is not set, the oldest event time in the EPG database is used.
        //! An EIT sections are regenerated when necessary.
        //! EIT p/f are saved first. Then EIT schedule.
        //! @param [in,out] sections A section file object into which all current EIT sections are saved.
        //!
        void saveEITs(SectionFile& sections);

        //!
        //! Dump the internal state of the EIT generator on the DuckContext Report object.
        //! @param [in] level Severity level at which the state is dumped.
        //!
        void dumpInternalState(int level) const;

    private:

        // -----------------------
        // Description of an event
        // -----------------------

        class Event
        {
            TS_NOBUILD_NOCOPY(Event);
        public:
            uint16_t  event_id = 0;   // Event id.
            Time      start_time {};  // Decoded event start time.
            Time      end_time {};    // Decoded event end time.
            ByteBlock event_data {};  // Binary event data, from event_id to end of descriptor loop.

            // Constructor based on EIT section payload: extract the next event.
            // The data and size are updated after building the event.
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
            bool       obsolete = false;  // The section is obsolete, discard it when found in an injection list.
            bool       injected = false;  // Indicate that the data part of the section is used in a packetizer.
            Time       next_inject {};    // Date of next injection.
            SectionPtr section {};        // Safe pointer to the EIT section.

            // Constructor, build an empty section for the specified service (CRC32 not set).
            ESection(EITGenerator* gen, const ServiceIdTriplet& service_id, TID tid, uint8_t section_number, uint8_t last_section_number);

            // Indicate that the section will be modified. It the section is or has recently been used in a
            // packetizer, a copy of the section is created first to avoid corrupting the section being packetized.
            void startModifying();

            // Toogle the actual/other status for the section.
            void toggleActual(bool actual);

            // Increment version of section. Does nothing when option SYNC_VERSIONS is set (versions are separately updated later).
            void updateVersion(EITGenerator* gen, bool recompute_crc);
        };

        typedef SafePtr<ESection> ESectionPtr;
        typedef std::list<ESectionPtr> ESectionList;      // a list of EIT schedule sections
        typedef std::array<ESectionPtr, 2> ESectionPair;  // a pair of EIT p/f sections

        // ------------------------------------------------------------------
        // Description of an EIT schedule segment (3 hours, up to 8 sections)
        // ------------------------------------------------------------------

        class ESegment
        {
            TS_NOBUILD_NOCOPY(ESegment);
        public:
            const Time   start_time;         // Segment start time (a multiple of 3 hours). Never change.
            bool         regenerate = true;  // Regenerate all EIT schedule sections in the segment.
                                             // Initially true since all segments must have at least one section.
            EventList    events {};          // List of events in the segment, sorted by start time.
            ESectionList sections {};        // Current list of sections in the segment, sorted by start time.

            // Constructor.
            ESegment(const Time& seg_start_time) : start_time(seg_start_time) {}
        };

        typedef SafePtr<ESegment> ESegmentPtr;
        typedef std::list<ESegmentPtr> ESegmentList;

        // ------------------------
        // Description of a service
        // ------------------------

        class EService
        {
            TS_NOCOPY(EService);
        public:
            bool         regenerate = false;  // Some segments must be regenerated in the service.
            ESectionPair pf {};               // EIT p/f sections (0: present, 1: following).
            ESegmentList segments {};         // List of 3-hour segments (EPG events and EIT schedule sections).

            // Constructor.
            EService() = default;
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

        DuckContext&         _duck;                      // TSDuck execution context.
        const PID            _eit_pid;                   // PID for input and generated EIT's.
        uint16_t             _actual_ts_id = 0;          // Actual transport stream id (to differentiate EIT actual and others).
        bool                 _actual_ts_id_set = false;  // Boolean: value in _actual_ts_id is valid.
        bool                 _regenerate = false;        // Some segments must be regenerated in some services.
        PacketCounter        _packet_index = 0;          // Packet counter in the TS.
        BitRate              _max_bitrate = 0;           // Max EIT bitrate.
        BitRate              _ts_bitrate = 0;            // Declared TS bitrate.
        Time                 _ref_time {};               // Last reference time.
        PacketCounter        _ref_time_pkt = 0;          // Packet index at last reference time.
        PacketCounter        _eit_inter_pkt = 0;         // Inter-packet distance in the EIT PID (zero if unbound).
        PacketCounter        _last_eit_pkt = 0;          // Packet index at last EIT insertion.
        EITOptions           _options = EITOptions::GEN_ALL; // EIT generation options flags.
        EITRepetitionProfile _profile {};                // EIT repetition profile.
        SectionDemux         _demux;                     // Section demux for input stream, get PAT, TDT, TOT, EIT.
        Packetizer           _packetizer;                // Packetizer for generated EIT's.
        EServiceMap          _services {};               // Map of services -> segments -> events and sections.
        ESectionListArray    _injects {};                // Arrays of sections for injection.
        MilliSecond          _section_gap = 30;          // Minimum gap between sections of the same tid/tidext, DVB specifies at least 25 ms.
        TID                  _last_tid = TID_NULL;       // TID of last injected section, or 0.
        uint16_t             _last_tidext = 0;           // TIDEXT of last injected section.
        size_t               _last_index = 0;            // Queue index of last injected section.
        size_t               _obsolete_count = 0;        // Number of obsolete sections in the injection lists.
        std::map<uint64_t,uint8_t> _versions {};         // Last version of sections.

        // Set a bitrate field and update EIT inter-packet.
        void setBitRateField(BitRate EITGenerator::* field, const BitRate& bitrate);

        // Update the EIT database according to the current time.
        // Obsolete events, sections and segments are discarded.
        // Segments which must be regenerated are marked as such (will be actually regenerated later, when used).
        void updateForNewTime(const Time& now);

        // Regenerate, if necessary, EIT p/f in a service. Return true if section is modified.
        void regeneratePresentFollowing(const ServiceIdTriplet& service_id, EService& srv, const Time& now);
        bool regeneratePresentFollowingSection(const ServiceIdTriplet& service_id, ESectionPtr& sec, TID tid, bool section_number, const EventPtr& event, const Time&inject_time);

        // Regenerate all EIT schedule, create missing segments and sections.
        void regenerateSchedule(const Time& now);

        // Compute the next version for a table. If option SYNC_VERSIONS is set, the section number is ignored.
        uint8_t nextVersion(const ServiceIdTriplet& service_id, TID table_id, uint8_t section_number);

        // Mark a section as obsolete, garbage collect obsolete sections if too many were not
        // naturally discarded from the injection lists. Also apply to entire segments.
        void markObsoleteSection(ESection& sec);
        void markObsoleteSegment(ESegment& seg);

        // Enqueue a section for injection.
        void enqueueInjectSection(const ESectionPtr& sec, const Time& next_inject, bool try_front);

        // Helper for dumpInternalState()
        void dumpSection(int level, const UString& margin, const ESectionPtr& section) const;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;
    };
}
