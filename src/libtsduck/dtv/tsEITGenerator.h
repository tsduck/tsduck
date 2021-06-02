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
#include "tsEITRepetitionProfile.h"
#include "tsSectionFile.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsTSPacket.h"
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! Generate and insert EIT sections based on an EPG content.
    //! @ingroup mpeg
    //!
    //! The EPG content is filled with generix EIT sections, either in binary or XML form.
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
        //! EIT generation options.
        //! The options can be specified as a byte mask.
        //!
        enum Options {
            EIT_ACTUAL = 0x0001,   //!< Generate EIT actual.
            EIT_OTHER  = 0x0002,   //!< Generate EIT other.
            EIT_PF     = 0x0004,   //!< Generate EIT present/following.
            EIT_SCHED  = 0x0008,   //!< Generate EIT schedule.
            EIT_ALL    = 0x000F,   //!< Generate all EIT's.
            EIT_INPUT  = 0x0010,   //!< Use input EIT's as EPG data.
        };

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside this object.
        //! @param [in] pid The PID containing EIT's to insert.
        //! @param [in] options EIT generation options.
        //! @param [in] profile The EIT repetition profile.
        //!
        explicit EITGenerator(DuckContext& duck,
                              PID pid = PID_EIT,
                              int options = EIT_ALL | EIT_INPUT,
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
        void setOptions(int options);

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
        //! Process one packet from the stream.
        //!
        //! @param [in,out] pkt A TS packet from the stream. If the packet belongs
        //! to the EIT PID or the null PID, it may be updated with new content.
        //!
        void processPacket(TSPacket& pkt);

        //!
        //! Load EPG data from all EIT sections in a section file.
        //! All events are individually extracted.
        //! @param [in] sections A section file object. Non-EIT sections are ignored.
        //!
        void loadEvents(const SectionFile& sections);

        //!
        //! Load EPG data from an EIT section.
        //! All events are individually extracted.
        //! @param [in] section A section object. Non-EIT sections are ignored.
        //!
        void loadEvents(const Section& section);

        //!
        //! Save all current EIT sections in a section file.
        //! @param [in,out] sections A section file object into which all current
        //! EIT sections are saved.
        //!
        void saveEITs(SectionFile& sections) const;

    private:
        DuckContext&          _duck;
        const PID             _eit_pid;
        uint16_t              _ts_id;
        bool                  _ts_id_set;
        int                   _options;
        EITRepetitionProfile  _profile;
        SectionDemux          _demux;
        CyclingPacketizer     _packetizer;
        std::list<SectionPtr> _sections;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;
    };
}
