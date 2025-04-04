//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Analyze EIT sections.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsSectionDemux.h"
#include "tsSignalizationDemux.h"
#include "tsShortEventDescriptor.h"
#include "tsExtendedEventDescriptor.h"
#include "tsLogicalChannelNumbers.h"
#include "tsAlgorithm.h"
#include "tsService.h"
#include "tsTime.h"
#include "tsEIT.h"
#include "tsSDT.h"
#include "tsNIT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class EITPlugin: public ProcessorPlugin,
                     private SignalizationHandlerInterface,
                     private TableHandlerInterface,
                     private SectionHandlerInterface
    {
        TS_PLUGIN_CONSTRUCTORS(EITPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

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

TS_REGISTER_PROCESSOR_PLUGIN(u"eit", ts::EITPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EITPlugin::EITPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze EIT sections", u"[options]")
{
    option(u"detailed", 'd');
    help(u"detailed", u"With --epg-dump, display detailed information on events.");

    option(u"epg-dump", 'e');
    help(u"epg-dump", u"Display the content of the EPG, all events, per service.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"Specify the output file for the report (default: standard output).");

    option(u"summary", 's');
    help(u"summary", u"Display a summary of EIT presence. This is the default if --epg-dump is not specified.");

    option(u"width", 'w', UNSIGNED);
    help(u"width", u"columns",
         u"Maximum line width for EPG dump. The default is " + UString::Decimal(_default_line_width) + u" columns. Zero means no line wrap.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::EITPlugin::getOptions()
{
    getPathValue(_outfile_name, u"output-file");
    _detailed = present(u"detailed");
    _epg_dump = present(u"epg-dump");
    _summary = present(u"summary") || !_epg_dump;
    getIntValue(_line_width, u"width", _default_line_width);
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::EITPlugin::start()
{
    // Create output file.
    if (!_outfile_name.empty()) {
        verbose(u"creating %s", _outfile_name);
        _outfile.open(_outfile_name, std::ios::out);
        if (!_outfile) {
            error(u"cannot create %s", _outfile_name);
            return false;
        }
    }

    // Reset analysis state
    _last_utc = Time::Epoch;
    _eitpf_act_count = 0;
    _eitpf_oth_count = 0;
    _eits_act_count = 0;
    _eits_oth_count = 0;
    _services.clear();
    _ts_id = INVALID_TS_ID;
    _sec_demux.reset();
    _sec_demux.addPID(PID_EIT);
    if (_epg_dump) {
        // Collect service names and LCN from other TS?
        _sec_demux.addPID(PID_SDT);
        _sec_demux.addPID(PID_NIT);
    }
    _sig_demux.reset();
    _sig_demux.addFullFilters();

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::EITPlugin::stop()
{
    std::ostream& out(_outfile.is_open() ? _outfile : std::cout);
    if (_epg_dump) {
        printEPG(out);
    }
    if (_epg_dump && _summary) {
        out << std::endl;
    }
    if (_summary) {
        printSummary(out);
    }
    if (_outfile.is_open()) {
        _outfile.close();
    }
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::EITPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    _sig_demux.feedPacket(pkt);
    _sec_demux.feedPacket(pkt);
    return TSP_OK;
}


//----------------------------------------------------------------------------
// Return a reference to a service description
//----------------------------------------------------------------------------

ts::EITPlugin::ServiceDesc& ts::EITPlugin::getServiceDesc(uint16_t ts_id, uint16_t service_id)
{
    uint32_t index = MakeIndex(ts_id, service_id);

    if (!_services.contains(index)) {
        verbose(u"new service %n, TS id %n", service_id, ts_id);
        ServiceDesc& serv(*(_services[index] = std::make_shared<ServiceDesc>()));
        serv.service.setId(service_id);
        serv.service.setTSId(ts_id);
        return serv;
    }
    else {
        ServiceDesc& serv(*_services[index]);
        assert(serv.service.hasId(service_id));
        assert(serv.service.hasTSId(ts_id));
        return serv;
    }
}


//----------------------------------------------------------------------------
// Return a reference to an event description.
//----------------------------------------------------------------------------

ts::EITPlugin::EventDesc& ts::EITPlugin::getEventDesc(ServiceDesc& serv, uint16_t event_id)
{
    if (!serv.events.contains(event_id)) {
        serv.events[event_id] = std::make_shared<EventDesc>();
        serv.events[event_id]->event_id = event_id;
    }
    return *serv.events[event_id];
}


//----------------------------------------------------------------------------
// Format string with line wraps.
//----------------------------------------------------------------------------

ts::UString ts::EITPlugin::wrapped(const UString& text, const UString& next_margin)
{
    if (_line_width == 0) {
        return text;
    }
    else {
        return text.toSplitLines(_line_width, UString(), next_margin);
    }
}


//----------------------------------------------------------------------------
// Print the EPG dump.
//----------------------------------------------------------------------------

void ts::EITPlugin::printEPG(std::ostream& out)
{
    // Build an ordered list of services. The order is based on LCN when available,
    // then on service names, and last on service ids.
    std::list<ServiceDescPtr> services(MapValuesList(_services));
    services.sort([](const ServiceDescPtr& a, const ServiceDescPtr& b)
    {
        if (a->service.hasLCN() && b->service.hasLCN()) {
            return a->service.getLCN() < b->service.getLCN();
        }
        else if (a->service.hasLCN() || b->service.hasLCN()) {
            return a->service.hasLCN();
        }
        else if (a->service.hasName() && b->service.hasName()) {
            return a->service.getName() < b->service.getName();
        }
        else if (a->service.hasName() || b->service.hasName()) {
            return a->service.hasName();
        }
        else {
            return a->service.getId() < b->service.getId();
        }
    });

    // Display all services with events.
    bool first = true;
    for (const auto& serv : services) {
        if (!serv->events.empty()) {

            // Build an ordered list of events by time in that service.
            std::list<EventDescPtr> events(MapValuesList(serv->events));
            events.sort([](const EventDescPtr& a, const EventDescPtr& b) { return a->start_time < b->start_time; });

            // Build service name.
            UString sname;
            if (serv->service.hasLCN()) {
                sname.format(u"%d. ", serv->service.getLCN());
            }
            if (serv->service.hasName() && !serv->service.getName().empty()) {
                sname.append(serv->service.getName());
            }
            else {
                sname.format(u"Service 0x%X, TS 0x%X", serv->service.getId(), serv->service.getTSId());
            }
            if (!first) {
                out << std::endl;
            }
            first = false;
            out << sname << std::endl << UString(sname.width(), u'-') << std::endl;

            // Display events.
            Time current_day = Time::Epoch;
            for (const auto& ev : events) {
                const Time day = ev->start_time.thisDay();
                if (day > current_day) {
                    out << day.format(Time::DATE) << std::endl;
                    current_day = day;
                }
                if (_detailed) {
                    out << wrapped(UString::Format(u"  %s to %s (%d mn), event id: %n",
                                                   ev->start_time.format(Time::TIME),
                                                   (ev->start_time + ev->duration).format(Time::TIME),
                                                   cn::duration_cast<cn::minutes>(ev->duration).count(),
                                                   ev->event_id)) << std::endl;
                    if (!ev->title.empty()) {
                        out << wrapped(u"    Title: " + ev->title) << std::endl;
                    }
                    if (!ev->short_text.empty()) {
                        out << wrapped(u"    Description: " + ev->short_text) << std::endl;
                    }
                    if (!ev->extended_text.empty()) {
                        out << wrapped(u"    Extended description: " + ev->extended_text) << std::endl;
                    }
                }
                else {
                    out << wrapped(UString::Format(u"  %s, %d mn, %s",
                                                   ev->start_time.format(Time::HOUR | Time::MINUTE),
                                                   cn::duration_cast<cn::minutes>(ev->duration).count(),
                                                   ev->title)) << std::endl;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Print the EPG summary.
//----------------------------------------------------------------------------

void ts::EITPlugin::printSummary(std::ostream& out)
{
    out << "Summary" << std::endl
        << "-------" << std::endl;
    if (_ts_id != INVALID_TS_ID) {
        out << UString::Format(u"TS id:         %n", _ts_id) << std::endl;
    }
    if (_last_utc != Time::Epoch) {
        out << "Last UTC:      " << _last_utc.format(Time::DATETIME) << std::endl;
    }
    out << "EITp/f actual: " << UString::Decimal(_eitpf_act_count) << std::endl
        << "EITp/f other:  " << UString::Decimal(_eitpf_oth_count) << std::endl
        << "EITs actual:   " << UString::Decimal(_eits_act_count) << std::endl
        << "EITs other:    " << UString::Decimal(_eits_oth_count) << std::endl
        << std::endl;

    // Summary by TS actual/other
    int scount_act = 0;
    int scount_oth = 0;
    int scount_eitpf_act = 0;
    int scount_eitpf_oth = 0;
    int scount_eits_act = 0;
    int scount_eits_oth = 0;
    cn::milliseconds max_time_act = cn::milliseconds::zero();
    cn::milliseconds max_time_oth = cn::milliseconds::zero();
    size_t name_width = 0;
    for (const auto& it : _services) {
        const ServiceDesc& serv(*it.second);
        name_width = std::max(name_width, serv.service.getName().width());
        if (_ts_id != INVALID_TS_ID && serv.service.hasTSId(_ts_id)) {
            // Actual TS
            scount_act++;
            if (serv.eitpf_count != 0) {
                scount_eitpf_act++;
            }
            if (serv.eits_count != 0) {
                scount_eits_act++;
            }
            max_time_act = std::max(max_time_act, serv.max_time);
        }
        else {
            // Other TS
            scount_oth++;
            if (serv.eitpf_count != 0) {
                scount_eitpf_oth++;
            }
            if (serv.eits_count != 0) {
                scount_eits_oth++;
            }
            max_time_oth = std::max(max_time_oth, serv.max_time);
        }
    }
    out << "TS      Services  With EITp/f  With EITs  EPG days" << std::endl
        << "------  --------  -----------  ---------  --------" << std::endl
        << UString::Format(u"Actual  %8d  %11d  %9d  %8d", scount_act, scount_eitpf_act, scount_eits_act, Days(max_time_act)) << std::endl
        << UString::Format(u"Other   %8d  %11d  %9d  %8d", scount_oth, scount_eitpf_oth, scount_eits_oth, Days(max_time_oth)) << std::endl
        << std::endl;

    // Summary by service
    const UString h_name(u"Name");
    name_width = std::max(name_width, h_name.length());
    out << UString::Format(u"A/O  TS Id   Srv Id  %-*s  EITp/f  EITs  EPG days", name_width, u"Name") << std::endl
        << UString::Format(u"---  ------  ------  %s  ------  ----  --------", UString(name_width, u'-')) << std::endl;
    for (const auto& it : _services) {
        const ServiceDesc& serv(*it.second);
        const bool actual = _ts_id != INVALID_TS_ID && serv.service.hasTSId(_ts_id);
        out << UString::Format(u"%s  0x%04X  0x%04X  %-*s  %-6s  %-4s  %8d",
                               actual ? u"Act" : u"Oth",
                               serv.service.getTSId(), serv.service.getId(),
                               name_width, serv.service.getName(),
                               UString::YesNo(serv.eitpf_count != 0),
                               UString::YesNo(serv.eits_count != 0),
                               Days(serv.max_time))
            << std::endl;
    }
}


//----------------------------------------------------------------------------
// Invoked when a new TS id, UTC time, or service info is available.
//----------------------------------------------------------------------------

void ts::EITPlugin::handleTSId(uint16_t ts_id, TID tid)
{
    _ts_id = ts_id;
}

void ts::EITPlugin::handleUTC(const Time& utc, TID tid)
{
    _last_utc = utc;
}

void ts::EITPlugin::handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed)
{
    getServiceDesc(ts_id, service.getId()).service.update(service);
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
// Used to collect service names and LCN from other TS.
// The SignalizationDemux collects them for the current TS only.
//----------------------------------------------------------------------------

void ts::EITPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {
        case TID_NIT_ACT:
        case TID_NIT_OTH: {
            const NIT nit(duck, table);
            if (nit.isValid()) {
                // Get all LCN definitions from that NIT, all TS.
                LogicalChannelNumbers lcn(duck);
                lcn.addFromNIT(nit);
                // Get the corresponding set of services.
                std::set<ServiceIdTriplet> sids;
                lcn.getServices(sids);
                // Create/update known services.
                for (const auto& sid : sids) {
                    lcn.updateService(getServiceDesc(sid.transport_stream_id, sid.service_id).service, true);
                }
            }
            break;
        }
        case TID_SDT_ACT:
        case TID_SDT_OTH: {
            const SDT sdt(duck, table);
            if (sdt.isValid()) {
                // Collect all service names from the SDT.
                for (const auto& sv : sdt.services) {
                    sv.second.updateService(duck, getServiceDesc(sdt.ts_id, sv.first).service);
                }
            }
            break;
        }
        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete EIT section is available.
// Because EIT's are segmented subtables, we analyse them by section.
//----------------------------------------------------------------------------

void ts::EITPlugin::handleSection(SectionDemux& demux, const Section& sect)
{
    const TID tid = sect.tableId();

    // Reject non-EIT sections.
    if (!sect.isValid() || tid < TID_EIT_MIN || tid > TID_EIT_MAX) {
        return;
    }

    // Use the section as if it was a complete table, to deserialize it as an EIT.
    SectionPtr newsec(new Section(sect, ShareMode::COPY));
    newsec->setSectionNumber(0, false);
    newsec->setLastSectionNumber(0, true);
    BinaryTable table;
    table.addSection(newsec);

    // Deserialize the EIT section.
    const EIT eit(duck, table);
    if (!eit.isValid()) {
        debug(u"received invalid EIT section, cannot be deserialized");
        return;
    }

    // Get service characteristics.
    ServiceDesc& serv(getServiceDesc(eit.ts_id, eit.service_id));

    // Check other/actual TS.
    if (_ts_id != INVALID_TS_ID) {
        if (eit.isActual() && !serv.service.hasTSId(_ts_id)) {
            verbose(u"EIT-Actual has wrong TS id %n", serv.service.getTSId());
        }
        else if (!eit.isActual() && serv.service.hasId(_ts_id)) {
            verbose(u"EIT-Other has same TS id as current TS");
        }
    }

    // Count EIT's for summary.
    if (_summary) {
        if (eit.isPresentFollowing()) {
            if (serv.eitpf_count++ == 0) {
                verbose(u"service %n, TS id %n, has EITp/f", serv.service.getId(), serv.service.getTSId());
            }
            if (eit.isActual()) {
                _eitpf_act_count++;
            }
            else {
                _eitpf_oth_count++;
            }
        }
        else {
            if (serv.eits_count++ == 0) {
                verbose(u"service %n, TS id %n, has EITs", serv.service.getId(), serv.service.getTSId());
            }
            if (eit.isActual()) {
                _eits_act_count++;
            }
            else {
                _eits_oth_count++;
            }
        }

        // Loop on all events in EIT schedule, compute time offset in the future
        if (!eit.isPresentFollowing() && _last_utc != Time::Epoch) {
            for (const auto& event : eit.events) {
                serv.max_time = std::max(serv.max_time, event.second.start_time - _last_utc);
            }
        }
    }

    // Store all events for later EPG dump.
    if (_epg_dump) {
        for (const auto& event : eit.events) {
            // Get event description in the plugin.
            EventDesc& ed(getEventDesc(serv, event.second.event_id));

            ed.start_time = event.second.start_time;
            ed.duration = event.second.duration;

            // Search name and description in the descriptor list.
            // The extended text is the concatenation of the texts in all extended_event_descriptor.
            UString extended_text;
            for (const auto& desc : event.second.descs) {
                if (desc.tag() == DID_DVB_SHORT_EVENT) {
                    ShortEventDescriptor sed(duck, desc);
                    if (sed.isValid()) {
                        ed.title = sed.event_name;
                        ed.short_text = sed.text;
                    }
                }
                else if (desc.tag() == DID_DVB_EXTENDED_EVENT) {
                    ExtendedEventDescriptor eed(duck, desc);
                    if (eed.isValid()) {
                        extended_text.append(eed.text);
                    }
                }
            }
            if (!extended_text.empty()) {
                ed.extended_text = extended_text;
            }
        }
    }
}
