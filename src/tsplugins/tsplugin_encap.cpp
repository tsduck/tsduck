//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
        bool                         _ignoreErrors = false;  // Ignore encapsulation errors.
        bool                         _pack = false;          // Outer packet packing option.
        size_t                       _packLimit = 0;         // Max limit distance.
        size_t                       _maxBuffered = 0;       // Max buffered packets.
        PID                          _pidOutput = PID_NULL;  // Output PID.
        PID                          _pidPCR = PID_NULL;     // PCR reference PID.
        PIDSet                       _pidsInput {};          // Input PID's.
        PacketEncapsulation::PESMode _pesMode = PacketEncapsulation::DISABLED;
        size_t                       _pesOffset = 0;         // Offset value in PES Synchronous.
        PacketEncapsulation          _encap {};              // Encapsulation engine.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"encap", ts::EncapPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EncapPlugin::EncapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Encapsulate packets from several PID's into one single PID", u"[options]")
{
    option(u"ignore-errors", 'i');
    help(u"ignore-errors",
         u"Ignore errors such as PID conflict or packet overflow. By default, a PID conflict is "
         u"reported when the output PID is already present on input but not encapsulated. "
         u"A packet overflow is reported when the input stream does not contain enough "
         u"null packets to absorb the encapsulation overhead.");

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

    option(u"pes-mode", 0, Enumeration({
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
         u"It requires to use the PCR option (--pcr-pid). "
         u"The value 0 is equivalent to use the Asynchronous PES encapsulation.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::EncapPlugin::getOptions()
{
    _ignoreErrors = present(u"ignore-errors");
    _pack = present(u"pack");
    getIntValue(_packLimit, u"pack", 0);
    getIntValue(_maxBuffered, u"max-buffered-packets", PacketEncapsulation::DEFAULT_MAX_BUFFERED_PACKETS);
    getIntValue(_pidOutput, u"output-pid", PID_NULL);
    getIntValue(_pidPCR, u"pcr-pid", PID_NULL);
    getIntValue(_pesMode, u"pes-mode", PacketEncapsulation::DISABLED);
    getIntValue(_pesOffset, u"pes-offset", 0);
    getIntValues(_pidsInput, u"pid");

    if (_pesOffset != 0 && _pesMode == PacketEncapsulation::DISABLED) {
        tsp->error(u"invalid use of pes-offset, it's only valid when PES mode is enabled.");
        return false;
    }
    if (_pesOffset != 0 && _pidPCR == PID_NULL) {
        tsp->error(u"invalid use of pes-offset, it's only valid when using pcr-pid.");
        return false;
    }

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::EncapPlugin::start()
{
    _encap.reset(_pidOutput, _pidsInput, _pidPCR);
    _encap.setPacking(_pack, _packLimit);
    _encap.setPES(_pesMode);
    _encap.setPESOffset(_pesOffset);
    _encap.setMaxBufferedPackets(_maxBuffered);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::EncapPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_encap.processPacket(pkt) || _ignoreErrors || _encap.lastError().empty()) {
        return TSP_OK;
    }
    else {
        tsp->error(_encap.lastError());
        return TSP_END;
    }
}
