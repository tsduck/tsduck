// Sample TSDuck extension.
// This file is included in the shared library.
// It exports the required symbol for this shared library to be recognized as a valid TSDuck extension.

#include "fooTable.h"

//----------------------------------------------------------------------------
// Plugin interface.
// Since this plugin is dedicated to the manipulation of a specific table, it
// is more convenient to derive it from the specialized AbstractTablePlugin
// class instead of the more general ProcessorPlugin superclass.
//----------------------------------------------------------------------------

namespace foo {
    class FootPlugin: public ts::AbstractTablePlugin
    {
        TS_NOBUILD_NOCOPY(FootPlugin);

    public:
        // Implementation of plugin API
        FootPlugin(ts::TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;

    protected:
        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(ts::BinaryTable& table) override;
        virtual void modifyTable(ts::BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

    private:
        ts::PID     _pid;       // PID for the table to process.
        bool        _set_id;    // If true, set a new id.
        bool        _set_name;  // If true, set a new name.
        uint16_t    _new_id;
        ts::UString _new_name;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"foot", foo::FootPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

foo::FootPlugin::FootPlugin(ts::TSP* t) :
    ts::AbstractTablePlugin(t, u"Perform various transformations on the FOOT", u"[options]", u"FOOT"),
    _pid(ts::PID_NULL),
    _set_id(false),
    _set_name(false),
    _new_id(0),
    _new_name()
{
    option(u"pid", 'p', PIDVAL, 1, 1);
    help(u"pid", u"Specify the PID on which the FOOT is expected. This option is required.");

    option(u"id", 'i', UINT16);
    help(u"id", u"Modify the foo_id in the FOOT with the specified value.");

    option(u"name", 'n', STRING);
    help(u"name", u"Modify the name in the FOOT with the specified value.");
}


//----------------------------------------------------------------------------
// Get option method
//----------------------------------------------------------------------------

bool foo::FootPlugin::getOptions()
{
    // Get option values
    _pid = intValue<ts::PID>(u"pid", ts::PID_NULL);
    _set_id = present(u"id");
    _set_name = present(u"name");
    _new_id = intValue<uint16_t>(u"id");
    _new_name = value(u"name");

    // Let superclass get its options.
    return ts::AbstractTablePlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool foo::FootPlugin::start()
{
    // Set table PID in superclass.
    setPID(_pid);

    // Start superclass.
    return ts::AbstractTablePlugin::start();
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void foo::FootPlugin::createNewTable(ts::BinaryTable& table)
{
    foo::FooTable foot;
    foot.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void foo::FootPlugin::modifyTable(ts::BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all)
{
    // If not a FOOT, reinsert without modification.
    is_target = table.tableId() == foo::TID_FOOT;
    if (!is_target) {
        return;
    }

    // Process the FOOT.
    foo::FooTable foot(duck, table);
    if (!foot.isValid()) {
        tsp->warning(u"found invalid FOOT");
        reinsert = false;
        return;
    }
    tsp->verbose(u"modifying a FOOT, PID 0x%X, foo_id: 0x%X", table.sourcePID(), foot.foo_id);

    // Modify global values.
    if (_set_id) {
        foot.foo_id = _new_id;
    }
    if (_set_name) {
        foot.name = _new_name;
    }

    // Reserialize modified FOOT.
    foot.serialize(duck, table);
}
