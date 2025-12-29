//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  FLUTE analyzer.
//
//----------------------------------------------------------------------------

#include "tsAbstractSingleMPEPlugin.h"
#include "tsPluginRepository.h"
#include "tsMPEPacket.h"
#include "tsmcastFluteAnalyzer.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts::mcast {
    class FlutePlugin: public AbstractSingleMPEPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(FlutePlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual void handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe) override;

    private:
        // Command line options.
        FluteAnalyzerArgs _opt_flute {};

        // Plugin private fields.
        FluteAnalyzer _flute_analyzer {duck};
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"flute", ts::mcast::FlutePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::mcast::FlutePlugin::FlutePlugin(TSP* tsp_) :
    AbstractSingleMPEPlugin(tsp_, u"FLUTE protocol analyzer", u"[options]", u"FLUTE stream")
{
    _opt_flute.defineArgs(*this);
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::mcast::FlutePlugin::getOptions()
{
    return AbstractSingleMPEPlugin::getOptions() && _opt_flute.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::mcast::FlutePlugin::start()
{
    return AbstractSingleMPEPlugin::start() && _flute_analyzer.reset(_opt_flute);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::mcast::FlutePlugin::stop()
{
    if (_opt_flute.summary) {
        _flute_analyzer.printSummary();
    }
    return true;
}


//----------------------------------------------------------------------------
// MPE packet processing method
//----------------------------------------------------------------------------

void ts::mcast::FlutePlugin::handleSingleMPEPacket(PCR timestamp, TimeSource source, const MPEPacket& mpe)
{
    const IPSocketAddress destination(mpe.destinationSocket());
    log(2, u"MPE packet on PID %n, for address %s, %d bytes", mpe.sourcePID(), destination, mpe.datagramSize());
    _flute_analyzer.feedPacket(timestamp, mpe.sourceSocket(), destination, mpe.udpMessage(), mpe.udpMessageSize());
}
