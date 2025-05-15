//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Duplicate PCR values from a PID into a new PCR-only PID.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsByteBlock.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PCRDuplicatePlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PCRDuplicatePlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        PID           _ref_pid_arg = PID_NULL;  // Reference PCR source.
        size_t        _ref_label = NPOS;        // Label which indicates the reference PID.
        PID           _new_pid = PID_NULL;      // New PID to create.

        // Working data.
        bool          _pending_pcr = false;     // Insert a new PCR when possible.
        bool          _pid_conflict = false;    // New PID alread found on input.
        PID           _ref_pid = PID_NULL;      // Current reference PCR source.
        PacketCounter _ref_packet = 0;          // Packet index of last PCR in reference PID.
        uint64_t      _ref_pcr = INVALID_PCR;   // Last PCR value in reference PID.
        PacketCounter _total_pcr = 0;           // Number of PCR's in input PID.
        PacketCounter _missed_pcr = 0;          // Number of input PCR's not duplicated in output PID.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pcrduplicate", ts::PCRDuplicatePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PCRDuplicatePlugin::PCRDuplicatePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Duplicate PCR values from a PID into a new PCR-only PID", u"[options]")
{
    option(u"new-pid", 'n', PIDVAL, 1, 1);
    help(u"new-pid",
         u"New PID to create into which PCR shall be duplicated. "
         u"This option is required, there is no default value.");

    option(u"reference-pid", 'r', PIDVAL);
    help(u"reference-pid",
         u"PID containing the reference PCR to duplicate. "
         u"At most one of --reference-pid and --reference-label shall be specified. "
         u"By default, use the first PID containing a PCR.");

    option(u"reference-label", 'l', INTEGER, 0, 0, 0, TSPacketLabelSet::MAX);
    help(u"reference-label",
         u"Packet label indicating the PID containing the reference PCR to duplicate. "
         u"Each time a packet with that label is encountered, the reference PID switches "
         u"to the PID of this packet, if different from the previous reference PID. "
         u"At most one of --reference-pid and --reference-label shall be specified. "
         u"By default, use the first PID containing a PCR.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::PCRDuplicatePlugin::getOptions()
{
    getIntValue(_new_pid, u"new-pid", PID_NULL);
    getIntValue(_ref_pid_arg, u"reference-pid", PID_NULL);
    getIntValue(_ref_label, u"reference-label", NPOS);

    if (_ref_pid_arg != PID_NULL && _ref_label > TSPacketLabelSet::MAX) {
        error(u"At most one of --reference-pid and --reference-label shall be specified.");
        return false;
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PCRDuplicatePlugin::start()
{
    _ref_pid = _ref_pid_arg;
    _ref_packet = 0;
    _ref_pcr = INVALID_PCR;
    _pending_pcr = false;
    _pid_conflict = false;
    _total_pcr = 0;
    _missed_pcr = 0;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PCRDuplicatePlugin::stop()
{
    verbose(u"%'d input PCR found, %'d could not be duplicated", _total_pcr, _missed_pcr);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PCRDuplicatePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // Track PID conflict once.
    if (pid == _new_pid && !_pid_conflict) {
        error(u"new PCR PID %n already exists in TS, stopping PCR duplication");
        _pid_conflict = true;
    }
    if (_pid_conflict) {
        return TSP_OK;
    }

    // If we get a null packet and one PCR needs to be created.
    if (_pending_pcr && pid == PID_NULL && _ref_pcr != INVALID_PCR) {

        // Template of a PCR-only packet.
        static const TSPacket pcr_packet {{
            // Header: PID=0, adaptation field only, no payload, CC=0
            0x47, 0x00, 0x00, 0x20,
            // Adaptation field size:
            183,
            // Adaptation field flags: PCR
            0x10,
            // PCR placeholder: 48 bits
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            // Adaptation field stuffing: 172 bytes 0xFF
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF,
        }};

        // Compute PCR value from the previous reference PCR value and the bitrate.
        // If the bitrate is unknown, keep the reference PCR, even though we know it is incorrect.
        const BitRate bitrate = tsp->bitrate();
        const uint64_t pcr = _ref_pcr + (bitrate == 0 ? 0 : (((tsp->pluginPackets() - _ref_packet) * PKT_SIZE_BITS * SYSTEM_CLOCK_FREQ) / bitrate).toInt());

        // Replace the PCR value. Start from a null packet. No need to care about continuity counters,
        // the packets have no payload and CC are incremented only on packets with payload.
        pkt = pcr_packet;
        pkt.setPID(_new_pid);
        pkt.setPCR(pcr);

        // No need to create a new PCR, until the next input PCR.
        _pending_pcr = false;
        return TSP_OK;
    }

    // Not interested in packets without PCR.
    if (!pkt.hasPCR()) {
        return TSP_OK;
    }

    // Process reference PID switching if the first PID with PCR is found or according to --reference-label.
    if ((_ref_pid == PID_NULL && _ref_label > TSPacketLabelSet::MAX) ||
        (_ref_label <= TSPacketLabelSet::MAX && pkt_data.hasLabel(_ref_label) && pid != _ref_pid && pid != PID_NULL))
    {
        // Switch to a new reference PID.
        verbose(u"using PID %n as PCR reference", pid);
        _ref_pid = pid;
        _ref_pcr = INVALID_PCR;
    }

    // Process input PCR.
    if (pid == _ref_pid) {
        // Count PCR.
        _total_pcr++;
        if (_pending_pcr) {
            // We should have injected one duplicated PCR but found no null packet to do so.
            _missed_pcr++;
        }
        _pending_pcr = true;
        _ref_pcr = pkt.getPCR();
        _ref_packet = tsp->pluginPackets();
    }

    return TSP_OK;
}
