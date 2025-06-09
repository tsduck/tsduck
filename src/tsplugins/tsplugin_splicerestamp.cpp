//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Restamp PTS in SCTE 35 splice information.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsSectionDemux.h"
#include "tsSignalizationDemux.h"
#include "tsSpliceInformationTable.h"
#include "tsPacketizer.h"
#include "tsAlgorithm.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SpliceRestampPlugin:
        public ProcessorPlugin,
        private TableHandlerInterface,
        private SignalizationHandlerInterface,
        private SectionProviderInterface
    {
        TS_PLUGIN_CONSTRUCTORS(SpliceRestampPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options:
        bool     _replace = false;           // Replace pts_adjustment field without adding previous value.
        bool     _continuous = false;        // Continuously recompute the adjustment between the old and new PCR PID's.
        PID      _pid_arg = PID_NULL;        // The splice PID to restamp.
        PID      _old_pcr_pid = PID_NULL;    // Previous PCR reference.
        PID      _new_pcr_pid = PID_NULL;    // New PCR reference.
        uint64_t _pts_adjustment = 0;        // Raw PTS adjustment to apply.
        uint64_t _rebase_pts = INVALID_PTS;  // Assume that the first PTS in the stream will be set to this value.

        // Working data:
        PID                     _splice_pid = PID_NULL;              // The actual splice PID to restamp.
        std::optional<uint64_t> _current_adjustment {};              // Current PTS adjustment to apply.
        uint64_t                _old_pcr = INVALID_PCR;              // Last PCR value in old clock reference PID.
        PacketCounter           _old_pcr_packet = 0;                 // Packet index of _old_pcr.
        uint64_t                _new_pcr = INVALID_PCR;              // Last PCR value in new clock reference PID.
        PacketCounter           _new_pcr_packet = 0;                 // Packet index of _new_pcr.
        SectionDemux            _section_demux {duck, this};         // Section filter for splice information.
        SignalizationDemux      _sig_demux {duck, this};             // Signalization demux to get PMT's.
        Packetizer              _packetizer {duck, PID_NULL, this};  // Regenerate modified splice sections.
        std::list<SectionPtr>   _sections {};                        // List of sections to inject.
        std::map<PID,uint64_t>  _first_pts {};                       // First PTS in each PID.
        std::set<PID>           _service_pids {};                    // Set of PID's in the same service as the splice PID.

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Implementation of SignalizationHandlerInterface.
        virtual void handlePMT(const PMT&, PID) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter, SectionPtr&) override;
        virtual bool doStuffing() override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"splicerestamp", ts::SpliceRestampPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SpliceRestampPlugin::SpliceRestampPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Restamp PTS in SCTE 35 splice information", u"[options]")
{
    option(u"continuous", 'c');
    help(u"continuous",
         u"With --old-pcr-pid and --new-pcr-pid, continuously recompute the PTS adjustment between the old and new clock references. "
         u"By default, the PTS adjustment is computed once only, using the first adjacent pair of old and new PCR values. "
         u"This is the preferred method when transcoding introduces a drift in muxing the old and new PCR PID's.");

    option(u"new-pcr-pid", 'n', PIDVAL);
    help(u"new-pcr-pid",
         u"Specify the PID carrying the PCR which must be used as the new reference clock by the splice commands on output. "
         u"Must be used with --old-pcr-pid.");

    option(u"old-pcr-pid", 'o', PIDVAL);
    help(u"old-pcr-pid",
         u"Specify the PID carrying the PCR which was used as reference clock by the splice commands on input. "
         u"Must be used with --new-pcr-pid.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the PID carrying SCTE-35 sections to restamp. "
         u"By default, the first SCTE-35 PID is selected.");

    option(u"rebase-pts", 0, UNSIGNED, 0, 1, 0, MAX_PTS_DTS);
    help(u"rebase-pts",
         u"Set pts_adjustment as if the first PTS in the stream was set to the specified value.");

    option(u"pts-adjustment", 'a', UNSIGNED, 0, 1, 0, MAX_PTS_DTS);
    help(u"pts-adjustment",
         u"Add the specified value to the pts_adjustment field in the splice sections.");

    option(u"replace", 'r');
    help(u"replace",
         u"Replace the value of the pts_adjustment field in the splice sections. "
         u"Ignore the previous value instead of adding it.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::SpliceRestampPlugin::getOptions()
{
    _replace = present(u"replace");
    _continuous = present(u"continuous");
    getIntValue(_pid_arg, u"pid", PID_NULL);
    getIntValue(_old_pcr_pid, u"old-pcr-pid", PID_NULL);
    getIntValue(_new_pcr_pid, u"new-pcr-pid", PID_NULL);
    getIntValue(_pts_adjustment, u"pts-adjustment", 0);
    getIntValue(_rebase_pts, u"rebase-pts", INVALID_PTS);

    if ((_old_pcr_pid == PID_NULL && _new_pcr_pid != PID_NULL) || (_old_pcr_pid != PID_NULL && _new_pcr_pid == PID_NULL)) {
        error(u"options --old-pcr-pid and --new-pcr-pid must be used together");
        return false;
    }
    if ((_old_pcr_pid != PID_NULL) + (_pts_adjustment != 0) + (_rebase_pts != INVALID_PTS) > 1) {
        error(u"--pts-adjustment, --rebase-ptr, --old-pcr-pid/--new-pcr-pid are mutually exclusive");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SpliceRestampPlugin::start()
{
    _splice_pid = _pid_arg;
    _old_pcr = INVALID_PCR;
    _old_pcr_packet = 0;
    _new_pcr = INVALID_PCR;
    _new_pcr_packet = 0;
    _first_pts.clear();
    _service_pids.clear();

    if (_old_pcr_pid != PID_NULL || _rebase_pts != INVALID_PTS) {
        // Don't know yet which adjustment to apply.
        _current_adjustment.reset();
    }
    else {
        _current_adjustment = _pts_adjustment;
    }

    _sig_demux.reset();
    _sig_demux.addFilteredTableId(TID_PMT);
    _section_demux.reset();
    _section_demux.setPIDFilter(NoPID());
    _packetizer.reset();
    _sections.clear();

    // Start demuxing on the splice PID if specified on the command line.
    if (_splice_pid != PID_NULL) {
        _section_demux.addPID(_splice_pid);
        _packetizer.setPID(_splice_pid);
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the signalization demux when a PMT is found.
//----------------------------------------------------------------------------

void ts::SpliceRestampPlugin::handlePMT(const PMT& pmt, PID)
{
    // If the splice PID is unknown, analyze all components in the PMT, looking for a splice PID.
    if (_splice_pid == PID_NULL) {
        for (const auto& it : pmt.streams) {
            if (it.second.stream_type == ST_SCTE35_SPLICE) {
                // This is a PID carrying splice information.
                _splice_pid = it.first;
                _section_demux.addPID(_splice_pid);
                _packetizer.setPID(_splice_pid);
                verbose(u"using splice PID %n", _splice_pid);
                break;
            }
        }
    }

    // With --rebase-pts, get the set of PID in the same service as the splice PID.
    if (_splice_pid != PID_NULL && pmt.streams.contains(_splice_pid)) {
        _service_pids = MapKeysSet(pmt.streams);
        debug(u"%d PID's in splice service", _service_pids.size());
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a splice information section is available.
//----------------------------------------------------------------------------

void ts::SpliceRestampPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    // Convert to a Splice Information Table.
    SpliceInformationTable sit(duck, table);
    if (sit.isValid()) {
        debug(u"processing splice table, adjust value: %s, first PTS count: %d", _current_adjustment.has_value(), _first_pts.size());

        // With --rebase-pts, compute the PTS adjustment at the first splice section.
        if (_rebase_pts != INVALID_PTS && !_current_adjustment.has_value() && !_first_pts.empty()) {
            // Get the lowest PTS value in the same service as the splice PID (or in the TS if the service is unknown).
            uint64_t min_pts = INVALID_PTS;
            for (const auto& it : _first_pts) {
                if (_service_pids.empty() || _service_pids.contains(it.first)) {
                    min_pts = std::min(min_pts, it.second);
                }
            }
            // The idea of --rebase-pts is that the current PTS "min_pts" will be rebased as "_rebase_pts".
            // Compute the required pts_adjustment.
            if (min_pts != INVALID_PTS) {
                _current_adjustment = min_pts > _rebase_pts ? PTS_DTS_SCALE - (min_pts - _rebase_pts) : _rebase_pts - min_pts;
                verbose(u"initial PTS adjustment is %'d", _current_adjustment.value());
                debug(u"lowest PTS is %n", min_pts);
            }
        }

        // Now adjust the PTS in the splice section.
        if (_current_adjustment.has_value()) {
            // Update PTS adjustment.
            if (_replace) {
                sit.pts_adjustment = _current_adjustment.value();
            }
            else {
                sit.pts_adjustment = (sit.pts_adjustment + _current_adjustment.value()) & PTS_DTS_MASK;
            }

            // Serialize the modified table and enqueue the section (only one, normally).
            BinaryTable bin;
            if (sit.serialize(duck, bin)) {
                for (size_t i = 0; i < bin.sectionCount(); ++i) {
                    _sections.push_back(bin.sectionAt(i));
                }
            }
        }
        else {
            // If the current PTS adjustment is not yet known, we prefer to drop the splice section.
            // Otherwise, we could propagate a splice section with an incorrect PTS and create holes
            // in the stream when the splice is processed.
            warning(u"dropped SCTE-35 section, PTS adjustment not yet known");
        }
    }
}


//----------------------------------------------------------------------------
// Shall we perform section stuffing right now?
//----------------------------------------------------------------------------

bool ts::SpliceRestampPlugin::doStuffing()
{
    // In splice PID's, all sections use stuffing.
    return true;
}


//----------------------------------------------------------------------------
// Invoked when the packetizer needs a new section to insert.
//----------------------------------------------------------------------------

void ts::SpliceRestampPlugin::provideSection(SectionCounter counter, SectionPtr& section)
{
    if (_sections.empty()) {
        // No section to provide.
        section.reset();
    }
    else {
        // Remove one section from the queue for insertion.
        section = _sections.front();
        _sections.pop_front();
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SpliceRestampPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // With --rebase-pts, we need to track the first PTS in each PID as long as we don't know the PTS adjustment.
    if (_rebase_pts != INVALID_PTS && !_current_adjustment.has_value() && pkt.hasPTS() && !_first_pts.contains(pid)) {
        _first_pts[pid] = pkt.getPTS();
    }

    // Collect PCR values in old and new clock references.
    if (pid != PID_NULL && _old_pcr_pid != PID_NULL && (_continuous || _old_pcr == INVALID_PCR || _new_pcr == INVALID_PCR) && pkt.hasPCR()) {
        bool got_pcr = false;
        if (pid == _old_pcr_pid) {
            _old_pcr = pkt.getPCR();
            _old_pcr_packet = tsp->pluginPackets();
            got_pcr = true;
        }
        else if (pid == _new_pcr_pid) {
            _new_pcr = pkt.getPCR();
            _new_pcr_packet = tsp->pluginPackets();
            got_pcr = true;
        }
        if (got_pcr && _old_pcr != INVALID_PCR && _new_pcr != INVALID_PCR) {
            // Need to recompute the PTS adjustment.
            // Adjust PCR values at the current packet.
            // If the bitrate is unknown, keep the PCR values, even though we know that they are slightly incorrect.
            uint64_t old_pcr = _old_pcr;
            uint64_t new_pcr = _new_pcr;
            const BitRate bitrate = tsp->bitrate();
            if (bitrate > 0) {
                if (_old_pcr_packet < _new_pcr_packet) {
                    // Adjust old PCR.
                    old_pcr += (((_new_pcr_packet - _old_pcr_packet) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate).toInt() % PCR_SCALE;
                }
                else {
                    // Adjust new PCR.
                    new_pcr += (((_old_pcr_packet - _new_pcr_packet) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate).toInt() % PCR_SCALE;
                }
            }
            const uint64_t pts_adjust = new_pcr >= old_pcr ?
                ((new_pcr - old_pcr) % PCR_SCALE) / SYSTEM_CLOCK_SUBFACTOR :
                PTS_DTS_SCALE - ((old_pcr - new_pcr) % PCR_SCALE) / SYSTEM_CLOCK_SUBFACTOR;
            if (!_current_adjustment.has_value()) {
                verbose(u"initial PTS adjustment is %'d", pts_adjust);
                debug(u"old PCR: %'d 0x%<012X, new PCR: %'d 0x%<012X", old_pcr, new_pcr);
            }
            _current_adjustment = pts_adjust;
        }
    }

    // As long as the splice PID is unknown, look for PMT's.
    // Also need the PMT with --rebase-pts as long as the PTS adjustment is unknown.
    if (_splice_pid == PID_NULL || (_rebase_pts != INVALID_PTS && !_current_adjustment.has_value())) {
        _sig_demux.feedPacket(pkt);
    }

    // Extract splice information.
    _section_demux.feedPacket(pkt);

    // Replace packets from splice PID or null PID using packetizer.
    if (pid == _splice_pid || pid == PID_NULL) {
        _packetizer.getNextPacket(pkt);
    }

    return TSP_OK;
}
