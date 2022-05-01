// Sample TSDuck plugin (packet processing).
// Simply count packets if --count is specified.

#include "tsduck.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SamplePlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(SamplePlugin);
    public:
        // Implementation of plugin API
        SamplePlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual BitRate getBitrate() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options, stay unchanged after getOptions():
        bool doCount;  // Option --count

        // Processing data:
        PacketCounter counter;  // Actual packet counter.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"sample", ts::SamplePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SamplePlugin::SamplePlugin(TSP* tsp_) :
   ProcessorPlugin(tsp_, u"Sample packet processor", u"[options]"),
   doCount(false),
   counter(0)
{
    option(u"count", 'c');
    help(u"count", u"Count packets");
}


//----------------------------------------------------------------------------
// Get command line options
//----------------------------------------------------------------------------

bool ts::SamplePlugin::getOptions()
{
    tsp->verbose(u"sample plugin: get options");

    doCount = present(u"count");
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SamplePlugin::start()
{
    tsp->verbose(u"sample plugin: start");

    counter = 0;
    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SamplePlugin::stop()
{
    tsp->verbose(u"sample plugin: stop");

    // Close resources, display final report, etc.
    if (doCount) {
        tsp->info(u"got %d packets", {counter});
    }

    return true;
}


//----------------------------------------------------------------------------
// New bitrate computation method, return zero if unknown
//----------------------------------------------------------------------------

ts::BitRate ts::SamplePlugin::getBitrate()
{
    // If the plugin recomputes the bitrate, implement here.
    // Otherwise, return zero, or simply don't override getBitrate().
    return 0;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SamplePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pktData)
{
    if (doCount) {
        counter++;
    }
    return TSP_OK;
}
