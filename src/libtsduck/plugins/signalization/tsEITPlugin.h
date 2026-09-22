//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin to analyze EIT sections.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsProcessorPlugin.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsSignalizationDemux.h"
#include "tsService.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Plugin to analyze EIT sections.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL EITPlugin: public ProcessorPlugin, private SignalizationHandlerInterface, private TableHandlerInterface, private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(EITPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual PacketProcessStatus processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of one event (for full EPG dump).
        class EventDesc
        {
        public:
            uint16_t    event_id = 0;
            UString     title {};
            UString     short_text {};     // from short_event_descriptor
            UString     extended_text {};  // from extended_event_descriptor
            Time        start_time {};
            cn::seconds duration {0};
        };

        // Map of events, by event id.
        using EventDescPtr = std::shared_ptr<EventDesc>;
        using EventDescMap = std::map<uint16_t, EventDescPtr>;

        // Description of one service.
        class ServiceDesc
        {
        public:
            Service          service {};
            SectionCounter   eitpf_count = 0;
            SectionCounter   eits_count = 0;
            cn::milliseconds max_time {};  // Max time ahead of current time for EIT
            EventDescMap     events {};
        };

        // Map of services, indexed by combination of TS id / service id.
        using ServiceDescPtr = std::shared_ptr<ServiceDesc>;
        using ServiceDescMap = std::map<uint32_t, ServiceDescPtr>;

        // Combination of TS id / service id into one 32-bit index
        static uint32_t MakeIndex(uint16_t ts_id, uint16_t service_id) { return (uint32_t(ts_id) << 16) | service_id; }
        static uint16_t GetTSId(uint32_t index) { return (index >> 16) & 0xFFFF; }
        static uint16_t GetServiceId(uint32_t index) { return index & 0xFFFF; }

        // Command line options.
        fs::path _outfile_name {};
        bool     _summary = false;
        bool     _epg_dump = false;
        bool     _detailed = false;
        size_t   _line_width = _default_line_width;
        static constexpr size_t _default_line_width = 80;

        // Working data.
        std::ofstream      _outfile {};
        Time               _last_utc {};  // Last UTC time seen in TDT
        SectionCounter     _eitpf_act_count = 0;
        SectionCounter     _eitpf_oth_count = 0;
        SectionCounter     _eits_act_count = 0;
        SectionCounter     _eits_oth_count = 0;
        SectionDemux       _sec_demux {duck, this, this};
        SignalizationDemux _sig_demux {duck, this};
        uint16_t           _ts_id = INVALID_TS_ID;
        ServiceDescMap     _services {};

        // Return a reference to a service or event description.
        ServiceDesc& getServiceDesc(uint16_t ts_id, uint16_t service_id);
        EventDesc& getEventDesc(ServiceDesc&, uint16_t event_id);

        // Print the EPG reports.
        void printEPG(std::ostream& out);
        void printSummary(std::ostream& out);

        // Format string with line wraps.
        UString wrapped(const UString& text, const UString& next_margin = u"    ");

        // Inherited methods.
        virtual void handleTable(SectionDemux& demux, const BinaryTable& table) override;
        virtual void handleSection(SectionDemux&, const Section&) override;
        virtual void handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed) override;
        virtual void handleTSId(uint16_t ts_id, TID tid) override;
        virtual void handleUTC(const Time& utc, TID tid) override;

        // Number of days in a duration, used for EPG depth
        static cn::days::rep Days(cn::milliseconds ms) { return cn::duration_cast<cn::days>(ms).count(); }
    };
}
