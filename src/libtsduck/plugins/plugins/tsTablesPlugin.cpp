//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTablesPlugin.h"
#include "tsPluginRepository.h"
#include "tsPluginEventData.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"tables", ts::TablesPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TablesPlugin::TablesPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Collect PSI/SI Tables", u"[options]")
{
    duck.defineArgsForCAS(*this);
    duck.defineArgsForPDS(*this);
    duck.defineArgsForStandards(*this);
    duck.defineArgsForTimeReference(*this);
    duck.defineArgsForCharset(*this);
    _logger.defineArgs(*this);
    _display.defineArgs(*this);

    option(u"event-code", 0, UINT32);
    help(u"event-code",
         u"This option is for C++, Java or Python developers only.\n\n"
         u"Signal a plugin event with the specified code for each section. "
         u"The event data is an instance of PluginEventData pointing to the section content. "
         u"Without --all-sections, an event is signaled for each section of complete new tables.");

    option(u"joint-termination", 'j');
    help(u"joint-termination",
         u"With --max-tables, when the final table is collected, perform a \"joint termination\" instead of unconditional termination. "
         u"See \"tsp --help\" for more details on \"joint termination\".");
}


//----------------------------------------------------------------------------
// Start /stop methods
//----------------------------------------------------------------------------

bool ts::TablesPlugin::getOptions()
{
    _signal_event = present(u"event-code");
    getIntValue(_event_code, u"event-code");
    _logger.setSectionHandler(_signal_event ? this : nullptr);
    tsp->useJointTermination(present(u"joint-termination"));
    return duck.loadArgs(*this) && _logger.loadArgs(duck, *this) && _display.loadArgs(duck, *this);
}

bool ts::TablesPlugin::start()
{
    _terminated = false;
    return _logger.open();
}

bool ts::TablesPlugin::stop()
{
    _logger.close();
    _logger.reportDemuxErrors(*this, Severity::Verbose);
    return true;
}


//----------------------------------------------------------------------------
// Called by the TablesLogger for each section
//----------------------------------------------------------------------------

void ts::TablesPlugin::handleSection(SectionDemux&, const Section& sect)
{
    // Signal application-defined event. The call to the application callbacks is synchronous.
    if (_signal_event) {
        PluginEventData data(sect.content(), sect.size());
        tsp->signalPluginEvent(_event_code, &data);
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TablesPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if (_terminated) {
        // Typically waiting for joint termination, pass packet without processing.
        return TSP_OK;
    }
    else {
        // Normal packet processing.
        _logger.feedPacket(pkt);
        _terminated = _logger.completed();
        // Process termination.
        if (!_terminated) {
            return TSP_OK;
        }
        else if (tsp->useJointTermination()) {
            tsp->jointTerminate();
            return TSP_OK;
        }
        else {
            return TSP_END;
        }
    }
}
