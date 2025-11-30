//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  DVB-NIP (Native IP) analyzer.
//
//----------------------------------------------------------------------------

#include "tsAbstractSingleMPEPlugin.h"
#include "tsPluginRepository.h"
#include "tsMPEPacket.h"
#include "tsmcastNIPAnalyzer.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts::mcast {
    class NIPPlugin: public AbstractSingleMPEPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(NIPPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual void handleSingleMPEPacket(PCR timestamp, const MPEPacket& mpe) override;

    private:
        // Command line options.
        NIPAnalyzerArgs _opt_nip {};

        // Plugin private fields.
        NIPAnalyzer _nip_analyzer {duck};
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"nip", ts::mcast::NIPPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::mcast::NIPPlugin::NIPPlugin(TSP* tsp_) :
    AbstractSingleMPEPlugin(tsp_, u"DVB-NIP (Native IP) analyzer", u"[options]", u"DVB-NIP stream")
{
    _opt_nip.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::mcast::NIPPlugin::getOptions()
{
    return AbstractSingleMPEPlugin::getOptions() && _opt_nip.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::mcast::NIPPlugin::start()
{
    return AbstractSingleMPEPlugin::start() && _nip_analyzer.reset(_opt_nip);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::mcast::NIPPlugin::stop()
{
    if (_opt_nip.summary) {
        _nip_analyzer.printSummary();
    }
    return true;
}


//----------------------------------------------------------------------------
// MPE packet processing method
//----------------------------------------------------------------------------

void ts::mcast::NIPPlugin::handleSingleMPEPacket(PCR timestamp, const MPEPacket& mpe)
{
    const IPSocketAddress destination(mpe.destinationSocket());
    log(2, u"MPE packet on PID %n, for address %s, %d bytes", mpe.sourcePID(), destination, mpe.datagramSize());
    _nip_analyzer.feedPacket(timestamp, mpe.sourceSocket(), destination, mpe.udpMessage(), mpe.udpMessageSize());
}
