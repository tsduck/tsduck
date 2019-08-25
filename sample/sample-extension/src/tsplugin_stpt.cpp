// Sample TSDuck extension.
// This file is included in the shared library.
// It exports the required symbol for this shared library to be recognized as a valid TSDuck extension.

#include "tsduck.h"

//----------------------------------------------------------------------------
// Plugin interface.
// Since this plugin is dedicated to the manipulation of a specific table,
// it is more convenient to derive it from the specialized AbstractTablePlugin
// class instead of the more general ProcessorPlugin superclass.
//----------------------------------------------------------------------------

class STPTPlugin: public ts::AbstractTablePlugin
{
    TS_NOBUILD_NOCOPY(STPTPlugin);

public:
    // Implementation of plugin API
    STPTPlugin(ts::TSP*);
    virtual bool getOptions() override;
    virtual bool start() override;

protected:
    // Implementation of AbstractTablePlugin.
    virtual void createNewTable(ts::BinaryTable& table) override;
    virtual void modifyTable(ts::BinaryTable& table, bool& is_target, bool& reinsert) override;

private:
    ts::PID _pid;  // PID for the table to process.
};

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(stpt, STPTPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

STPTPlugin::STPTPlugin(ts::TSP* t) :
    ts::AbstractTablePlugin(t, u"Perform various transformations on the STPT", u"[options]", u"STPT"),
    _pid(ts::PID_NULL)
{
    option(u"pid", 'p', PIDVAL, 1, 1);
    help(u"pid", u"Specify the PID on which the STPT is expected. This option is required.");
}


//----------------------------------------------------------------------------
// Get option method
//----------------------------------------------------------------------------

bool STPTPlugin::getOptions()
{
    // Get option values
    _pid = intValue<ts::PID>(u"pid", ts::PID_NULL);

    // Let superclass get its options.
    return ts::AbstractTablePlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool STPTPlugin::start()
{
    // Set table PID in superclass.
    setPID(_pid);

    // Start superclass.
    return ts::AbstractTablePlugin::start();
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void STPTPlugin::createNewTable(ts::BinaryTable& table)
{
    //@@@@ STPT stpt;
    //@@@@ stpt.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void STPTPlugin::modifyTable(ts::BinaryTable& table, bool& is_target, bool& reinsert)
{
    //@@@@@
}
