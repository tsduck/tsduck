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
//  Encapsulate TS packets from several PID's into one single PID.
//  See also tsplugin_decap.cpp
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsPacketEncapsulation.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class EncapPlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        EncapPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool                _ignoreErrors;  // Ignore encapsulation errors.
        bool                _pack;          // Outer packet packing option.
        size_t              _maxBuffered;   // Max buffered packets.
        PID                 _pidOutput;     // Output PID.
        PID                 _pidPCR;        // PCR reference PID.
        PIDSet              _pidsInput;     // Input PID's.
        PacketEncapsulation _encap;         // Encapsulation engine.

        // Inaccessible operations
        EncapPlugin() = delete;
        EncapPlugin(const EncapPlugin&) = delete;
        EncapPlugin& operator=(const EncapPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(encap, ts::EncapPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::EncapPlugin::EncapPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Encapsulate packets from several PID's into one single PID", u"[options]"),
    _ignoreErrors(false),
    _pack(false),
    _maxBuffered(0),
    _pidOutput(PID_NULL),
    _pidPCR(PID_NULL),
    _pidsInput(),
    _encap()
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

    option(u"pack");
    help(u"pack",
         u"Emit outer packets when they are full only. By default, emit outer packets "
         u"as soon as possible, when null packets are available on input. With the default "
         u"behavior, inner packets are decapsulated with a better time accuracy, at the expense "
         u"of a higher bitrate of the outer PID when there are many null packets in input.");

    option(u"pid", 'p', INTEGER, 1, UNLIMITED_COUNT, 0, PID_NULL - 1);
    help(u"pid", u"pid1[-pid2]",
         u"Specify an input PID or range of PID's to encapsulate. "
         u"Several --pid options can be specified. "
         u"The null PID 0x1FFF cannot be encapsulated.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::EncapPlugin::getOptions()
{
    _ignoreErrors = present(u"ignore-errors");
    _pack = present(u"pack");
    _maxBuffered = intValue<size_t>(u"max-buffered-packets", PacketEncapsulation::DEFAULT_MAX_BUFFERED_PACKETS);
    _pidOutput = intValue<PID>(u"output-pid", PID_NULL);
    _pidPCR = intValue<PID>(u"pcr-pid", PID_NULL);
    getIntValues(_pidsInput, u"pid");

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::EncapPlugin::start()
{
    _encap.reset(_pidOutput, _pidsInput, _pidPCR);
    _encap.setPacking(_pack);
    _encap.setMaxBufferedPackets(_maxBuffered);
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::EncapPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    if (_encap.processPacket(pkt) || _ignoreErrors || _encap.lastError().empty()) {
        return TSP_OK;
    }
    else {
        tsp->error(_encap.lastError());
        return TSP_END;
    }
}
