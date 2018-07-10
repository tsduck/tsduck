// Sample TSDuck plugin (packet processing).
// Simpley count packets if --count is specified.

#include "tsduck.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SamplePlugin: public ProcessorPlugin
    {
    public:
        // Implementation of plugin API
        SamplePlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual BitRate getBitrate() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        // Private fields.
        bool          doCount;  // Option --count
        PacketCounter counter;  // Actual packet counter.

        // Inaccessible operations
        SamplePlugin() = delete;
        SamplePlugin(const SamplePlugin&) = delete;
        SamplePlugin& operator=(const SamplePlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(sample, ts::SamplePlugin)


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
// Start method
//----------------------------------------------------------------------------

bool ts::SamplePlugin::start()
{
    // Get command line options.
    doCount = present(u"count");

    // Initialize resources.
    counter = 0;

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::SamplePlugin::stop()
{
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

ts::ProcessorPlugin::Status ts::SamplePlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    if (doCount) {
        counter++;
    }

    return TSP_OK;
}
