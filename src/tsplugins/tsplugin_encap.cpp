//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Encapsulate TS packets from several PID's into one single PID.
//  See also tsplugin_decap.cpp
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPacketEncapsulation.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class EncapPlugin: public ProcessorPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(EncapPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        bool                         _ignore_errors = false;  // Ignore encapsulation errors.
        bool                         _pack = false;           // Outer packet packing option.
        bool                         _drop_initial = false;   // Drop initial input packet before the first PCR.
        size_t                       _pack_limit = 0;         // Max limit distance.
        size_t                       _max_buffered = 0;       // Max buffered packets.
        PID                          _output_pid = PID_NULL;  // Output PID.
        PID                          _pcr_pid = PID_NULL;     // PCR reference PID.
        size_t                       _pcr_label = NPOS;       // PCR reference label.
        PIDSet                       _input_pids {};          // Input PID's.
        TSPacketLabelSet             _input_labels {};        // Input packet labels.
        PacketEncapsulation::PESMode _pes_mode = PacketEncapsulation::DISABLED;
        int32_t                      _pes_offset = 0;         // Offset value in PES Synchronous.
        PacketEncapsulation          _encap {*this};          // Encapsulation engine.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"encap", ts::EncapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EncapPlugin::EncapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Encapsulate packets from several PID's into one single PID", u"[options]")
{
    option(u"drop-initial");
    help(u"drop-initial",
         u"In synchronous PES mode, all outer packets must contain a PTS. "
         u"However, a PTS cannot be computed before getting the first PCR. "
         u"If initial input packets arrive before the first PCR, they cannot be immediately encapsulated. "
         u"By default, they are delayed until the first PCR is found, when PTS can be computed. "
         u"Using this option, these initial input packets are dropped instead of being delayed.");

    option(u"ignore-errors", 'i');
    help(u"ignore-errors",
         u"Ignore errors such as PID conflict or packet overflow. By default, a PID conflict is "
         u"reported when the output PID is already present on input but not encapsulated. "
         u"A packet overflow is reported when the input stream does not contain enough "
         u"null packets to absorb the encapsulation overhead.");

    option(u"label", 'l', INTEGER, 0, UNLIMITED_COUNT, 0, TSPacketLabelSet::MAX);
    help(u"label", u"label1[-label2]",
         u"Encapsulate packets with the specified labels. "
         u"Several --label options may be specified.");

    option(u"max-buffered-packets", 'm', UNSIGNED);
    help(u"max-buffered-packets",
         u"Specify the maximum number of buffered packets. "
         u"The buffered packets are produced by the encapsulation overhead. "
         u"An overflow is usually caused by insufficient null packets in the input stream. "
         u"The default is " + UString::Decimal(PacketEncapsulation::DEFAULT_MAX_BUFFERED_PACKETS) + u" packets.");

    option(u"output-pid", 'o', INTEGER, 1, 1, 0, PID_NULL - 1);
    help(u"output-pid",
         u"Specify the output PID containing all encapsulated PID's. "
         u"This is a mandatory parameter, there is no default. "
         u"The null PID 0x1FFF cannot be the output PID.");

    option(u"pcr-label", 0, INTEGER, 0, 0, 0, TSPacketLabelSet::MAX);
    help(u"pcr-label",
         u"Specify a label for reference packets containing PCR's. The output PID will contain PCR's, "
         u"based on the same clock. By default, the output PID does not contain any PCR.");

    option(u"pcr-pid", 0, PIDVAL);
    help(u"pcr-pid",
         u"Specify a reference PID containing PCR's. The output PID will contain PCR's, "
         u"based on the same clock. By default, the output PID does not contain any PCR.");

    option(u"pack", 0, INTEGER, 0, 1, 0, UNLIMITED_VALUE, true);
    help(u"pack",
         u"Emit outer packets when they are full only. By default, emit outer packets "
         u"as soon as possible, when null packets are available on input. With the default "
         u"behavior, inner packets are decapsulated with a better time accuracy, at the expense "
         u"of a higher bitrate of the outer PID when there are many null packets in input. "
         u"You can limit the distance between packets adding a positive value. "
         u"With a 0 value the distance is disabled (=unlimited). "
         u"The value 1 is equivalent to not use the pack mode.");

    option(u"pid", 'p', INTEGER, 1, UNLIMITED_COUNT, 0, PID_NULL - 1);
    help(u"pid", u"pid1[-pid2]",
         u"Specify an input PID or range of PID's to encapsulate. "
         u"Several --pid options can be specified. "
         u"The null PID 0x1FFF cannot be encapsulated.");

    option(u"pes-mode", 0, Names({
        {u"disabled", PacketEncapsulation::DISABLED},
        {u"fixed",    PacketEncapsulation::FIXED},
        {u"variable", PacketEncapsulation::VARIABLE},
    }));
    help(u"pes-mode", u"mode", u"Enable PES mode encapsulation.");

    option(u"pes-offset", 0, INT32);
    help(u"pes-offset",
         u"Offset used in Synchronous PES mode encapsulation. "
         u"The value (positive or negative) is added to the current PCR to generate "
         u"the PTS timestamp inserted in the PES header. "
         u"The recommended values are between -90000 and +90000 (1 second). "
         u"It requires to use the PCR option (--pcr-pid or --pcr-label). "
         u"The value 0 is equivalent to use the Asynchronous PES encapsulation.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::EncapPlugin::getOptions()
{
    _ignore_errors = present(u"ignore-errors");
    _pack = present(u"pack");
    _drop_initial = present(u"drop-initial");
    getIntValue(_pack_limit, u"pack", 0);
    getIntValue(_max_buffered, u"max-buffered-packets", PacketEncapsulation::DEFAULT_MAX_BUFFERED_PACKETS);
    getIntValue(_output_pid, u"output-pid", PID_NULL);
    getIntValue(_pcr_pid, u"pcr-pid", PID_NULL);
    getIntValue(_pcr_label, u"pcr-label", NPOS);
    getIntValue(_pes_mode, u"pes-mode", PacketEncapsulation::DISABLED);
    getIntValue(_pes_offset, u"pes-offset", 0);
    getIntValues(_input_pids, u"pid");
    getIntValues(_input_labels, u"label");

    if (_pes_offset != 0 && _pes_mode == PacketEncapsulation::DISABLED) {
        error(u"invalid use of pes-offset, it's only valid when PES mode is enabled.");
        return false;
    }
    if (_pes_offset != 0 && _pcr_pid == PID_NULL && _pcr_label > TSPacketLabelSet::MAX) {
        error(u"invalid use of pes-offset, it's only valid when using --pcr-pid or --pcr-label.");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::EncapPlugin::start()
{
    _encap.reset(_output_pid, _input_pids, _input_labels, _pcr_pid, _pcr_label);
    _encap.setPacking(_pack, _pack_limit);
    _encap.setPES(_pes_mode);
    _encap.setPESOffset(_pes_offset);
    _encap.setMaxBufferedPackets(_max_buffered);
    _encap.setInitialPacketDrop(_drop_initial);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::EncapPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_encap.processPacket(pkt, pkt_data) || _ignore_errors || _encap.lastError().empty()) {
        return TSP_OK;
    }
    else {
        error(_encap.lastError());
        return TSP_END;
    }
}
