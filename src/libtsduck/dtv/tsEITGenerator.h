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
#include "tsCyclingPacketizer.h"
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
        PF_ACTUAL          = 0,   //! EIT present/following actual.
        PF_OTHER           = 1,   //! EIT present/following other.
        SCHED_ACTUAL_PRIME = 2,   //! EIT schedule actual in the "prime" period.
        SCHED_OTHER_PRIME  = 3,   //! EIT schedule other in the "prime" period.
        SCHED_ACTUAL_LATER = 4,   //! EIT schedule actual after the "prime" period.
        SCHED_OTHER_LATER  = 5,   //! EIT schedule other after the "prime" period.
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
}

namespace ts {
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
    //! The EPG content is filled with generic EIT sections, either in binary or XML form.
    //! The structure of EIT (p/f or schedule), the number and order of events, do not
    //! matter. The events are extracted and will be stored and sorted independently.
    //! These events will be used to fill the generated EIT sections.
    //!
    //! The object is continuously invoked for all packets in a TS.
    //! Packets from the EIT PID or the stuffing PID are replaced.
    //!
    class TSDUCKDLL EITGenerator : private SectionHandlerInterface
    {
        TS_NOBUILD_NOCOPY(EITGenerator);
    public:

        //!
        //! Constructor.
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
        //! The EPG content is deleted. The TS id is forgotten.
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
        //! Set the current time in the stream processing.
        //! By default, the current time is synchronized on each input TDT or TOT.
        //! Calling this method is useful only when there is no TDT/TOT or to set
        //! a time reference before the first TDT or TOT.
        //! @param [in] current_utc Current UTC time in the context of the stream.
        //!
        void setCurrentTime(Time current_utc);

        //!
        //! Get the current time in the stream processing.
        //! The current time starts from the last reference clock (TDT, TOT or setCurrentTime())
        //! and is maintained from the declared bitrate and number of processed packets.
        //! If there is no declared bitrate, the last reference time is returned.
        //! @param [in] current_utc Current UTC time in the context of the stream.
        //!
        Time getCurrentTime(Time current_utc) const;

        //!
        //! Set the current bitrate of the stream.
        //! The bitrate is used to update the current time, packet after packet,
        //! from the last reference clock (TDT, TOT or setCurrentTime()).
        //! @param [in] bitrate Current transport stream bitrate in bits per second.
        //! @see setCurrentTime()
        //!
        void setBitRate(BitRate bitrate) { _bitrate = bitrate; }

        //!
        //! Process one packet from the stream.
        //! @param [in,out] pkt A TS packet from the stream. If the packet belongs
        //! to the EIT PID or the null PID, it may be updated with new content.
        //!
        void processPacket(TSPacket& pkt);

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
        //! Load EPG data from all EIT sections in a section file.
        //! @param [in] sections A section file object. Non-EIT sections are ignored.
        //! @see loadEvents(const Section&)
        //!
        void loadEvents(const SectionFile& sections);

        //!
        //! Save all current EIT sections in a section file.
        //! @param [in,out] sections A section file object into which all current
        //! EIT sections are saved.
        //!
        void saveEITs(SectionFile& sections) const;

    private:
        // An internal structure to store binary events from sections.
        struct BinaryEvent
        {
            Time      start_time;  // Decoded event start time.
            Time      end_time;    // Decoded event end time.
            ByteBlock event_data;  // Binary event data, from event_id to end of descriptor loop.

            // Constructor based on EIT section payload. The data and size are updated after building the event.
            BinaryEvent(const uint8_t*& data, size_t& size);
        };

        // Use a map from service id to list of safe pointers to BinaryEvent (sorted by start time).
        typedef SafePtr<BinaryEvent> BinaryEventPtr;
        typedef std::list<BinaryEventPtr> BinaryEventList;
        typedef std::map<ServiceIdTriplet, BinaryEventList> BinaryEventMap;

        // EITGenerator private fields.
        DuckContext&          _duck;          // TSDuck execution context.
        const PID             _eit_pid;       // PID for input and generated EIT's.
        uint16_t              _ts_id;         // Transport stream id (to differentiate EIT actual and others).
        bool                  _ts_id_set;     // Boolean: value in _ts_id is valid.
        PacketCounter         _packet_index;  // Packet counter in the TS.
        BitRate               _bitrate;       // Declared TS bitrate.
        Time                  _ref_time;      // Last reference time.
        PacketCounter         _ref_time_pkt;  // Packet index at last reference time.
        EITOption             _options;       // EIT generation options flags.
        EITRepetitionProfile  _profile;       // EIT repetition profile.
        SectionDemux          _demux;         // Section demux for input stream, get PAT, TDT, TOT, EIT.
        CyclingPacketizer     _packetizer;    // Packetizer for generated EIT's.
        std::list<SectionPtr> _sections;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;
    };
}
