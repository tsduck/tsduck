//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//
//  Transport stream processor shared library:
//  Remove or merge sections from various PID's.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsPacketizer.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SectionsPlugin:
        public ProcessorPlugin,
        private SectionHandlerInterface,
        private SectionProviderInterface
    {
    public:
        // Implementation of plugin API
        SectionsPlugin(TSP*);
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool                  _section_stuffing;
        bool                  _use_null_pid;
        size_t                _max_buffered_sections;
        PIDSet                _input_pids;
        PID                   _output_pid;
        std::list<SectionPtr> _sections;
        std::set<TID>         _removed_tids;
        std::set<uint32_t>    _removed_etids;
        SectionDemux          _demux;
        Packetizer            _packetizer;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;

        // Inaccessible operations
        SectionsPlugin() = delete;
        SectionsPlugin(const SectionsPlugin&) = delete;
        SectionsPlugin& operator=(const SectionsPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(sections, ts::SectionsPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SectionsPlugin::SectionsPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Remove or merge sections from various PID's", u"[options]"),
    _section_stuffing(false),
    _use_null_pid(false),
    _max_buffered_sections(1024), // hard-coded for now
    _input_pids(),
    _output_pid(PID_NULL),
    _sections(),
    _removed_tids(),
    _removed_etids(),
    _demux(nullptr, this),
    _packetizer(PID_NULL, this)
{
    option(u"etid-remove", 'e', UINT32, 0, UNLIMITED_COUNT, 0, 0x00FFFFFF);
    help(u"etid-remove", u"id1[-id2]",
         u"Remove all sections with the corresponding \"extended table id\" values. "
         u"The value is a combination of the table id and the table id extension. "
         u"For example, the option -e 0x4A1234 removes all BAT sections (table id 0x4A) "
         u"for bouquet id 0x1234 (table id extension). "
         u"Several options --etid-remove can be specified.");

    option(u"null-pid-reuse", 'n');
    help(u"null-pid-reuse",
         u"With this option, null packets can be replaced by packets for the "
         u"output PID. By default, only packets from input PID's are replaced "
         u"by output packets. This option may need to be used when --stuffing "
         u"is specified and the input PID's contained packed sections. In that "
         u"case, the output payload can be larger than the input and additional "
         u"packets must be used.");

    option(u"output-pid", 'o', PIDVAL);
    help(u"output-pid",
         u"Specifies the output PID. By default, the first input PID on the "
         u"command line is used. If the output PID is different from all input "
         u"PID's and this output PID already exists in the transport stream, "
         u"an error is generated.");

    option(u"pid", 'p', PIDVAL, 1, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specify input PID's. More than one input PID can be specified. "
         u"All sections from all input PID's are merged into the output PID. "
         u"At least one input PID must be specified. ");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Insert stuffing at end of each section, up to the next TS packet "
         u"boundary. By default, sections are packed and start in the middle "
         u"of a TS packet, after the previous section. Note, however, that "
         u"section headers are never scattered over a packet boundary.");

    option(u"tid-remove", 't', UINT8, 0, UNLIMITED_COUNT);
    help(u"tid-remove", u"id1[-id2]",
         u"Remove all sections with the corresponding table id. "
         u"Several options --tid-remove can be specified.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SectionsPlugin::start()
{
    // Get option values
    _section_stuffing = present(u"stuffing");
    _use_null_pid = present(u"null-pid-reuse");
    _output_pid = intValue(u"output-pid", intValue<PID>(u"pid", PID_NULL, 0));
    getIntValues(_input_pids, u"pid");
    getIntValues(_removed_tids, u"tid-remove");
    getIntValues(_removed_etids, u"etid-remove");

    // Reset plugin state.
    _demux.reset();
    _demux.setPIDFilter(_input_pids);
    _packetizer.reset();
    _packetizer.setPID(_output_pid);
    _sections.clear();

    return true;
}


//----------------------------------------------------------------------------
// Shall we perform section stuffing right now?
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

bool ts::SectionsPlugin::doStuffing()
{
    return _section_stuffing;
}


//----------------------------------------------------------------------------
// Invoked when the packetizer needs a new section to insert.
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

void ts::SectionsPlugin::provideSection(SectionCounter counter, SectionPtr& section)
{
    if (_sections.empty()) {
        // No section to provide.
        section.clear();
    }
    else {
        // Remove one section from the queue for insertion.
        section = _sections.front();
        _sections.pop_front();
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete section is available.
// Implementation of SectionHandlerInterface.
//----------------------------------------------------------------------------

void ts::SectionsPlugin::handleSection(SectionDemux& demux, const Section& section)
{
    // Section characteristics.
    const TID tid = section.tableId();
    const uint32_t etid = (uint32_t(tid) << 16) | section.tableIdExtension();

    // Filter out sections to be removed.
    if (_removed_tids.find(tid) != _removed_tids.end() || (section.isLongSection() && _removed_etids.find(etid) != _removed_etids.end())) {
        return;
    }

    // At this point, we need to keep the section.
    // Build a copy of it for insertion in the queue.
    const SectionPtr sp(new Section(section, SHARE));
    CheckNonNull(sp.pointer());

    // Now insert the section in the queue for the packetizer.
    _sections.push_back(sp);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SectionsPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // If the output PID is not an input one and already exists, this is an error.
    if (pid == _output_pid && !_input_pids.test(_output_pid)) {
        tsp->error(u"output PID 0x%X (%d) already present in the stream", {_output_pid, _output_pid});
        return TSP_END;
    }

    // Filter sections to process / merge.
    _demux.feedPacket(pkt);

    // Fool-proof check. It the input PID's contain packed sections and
    // we perform section stuffing and we do not reuse null packets or
    // there are not enough null packets, we may accumulate more and
    // more sections until the memory is exhausted.
    if (_sections.size() > _max_buffered_sections) {
        tsp->error(u"too many accumulated buffered sections, not enough space in output PID");
        return TSP_END;
    }

    // Replace packets from all input PID's using packetizer.
    if (_input_pids.test(pid) || (_use_null_pid && pid == PID_NULL)) {
        _packetizer.getNextPacket(pkt);
    }

    return TSP_OK;
}
