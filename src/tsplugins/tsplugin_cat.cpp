//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Various transformations on the CAT.
//
//----------------------------------------------------------------------------

#include "tsAbstractTablePlugin.h"
#include "tsPluginRepository.h"
#include "tsCADescriptor.h"
#include "tsCAT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CATPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(CATPlugin);
    public:
        // Implementation of plugin API
        virtual bool getOptions() override;

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

    private:
        // Command line options:
        bool                  _cleanup_priv_desc = false; // Remove private desc without preceding PDS desc
        std::vector<uint16_t> _remove_casid {};           // Set of CAS id to remove
        std::vector<uint16_t> _remove_pid {};             // Set of EMM PID to remove
        DescriptorList        _add_descs {nullptr};       // List of descriptors to add
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"cat", ts::CATPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CATPlugin::CATPlugin (TSP* tsp_) :
    AbstractTablePlugin(tsp_, u"Perform various transformations on the CAT", u"[options]", u"CAT", PID_CAT)
{
    option(u"add-ca-descriptor", 'a', STRING, 0, UNLIMITED_COUNT);
    help(u"add-ca-descriptor", u"casid/pid[/private-data]",
         u"Add a CA_descriptor in the CAT with the specified CA System Id and "
         u"EMM PID. The optional private data must be a suite of hexadecimal digits. "
         u"Several --add-ca-descriptor options may be specified to add several "
         u"descriptors.");

    option(u"cleanup-private-descriptors");
    help(u"cleanup-private-descriptors",
         u"Remove all private descriptors without preceding private_data_specifier descriptor.");

    option(u"remove-casid", 'r', UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-casid", u"casid1[-casid2]",
         u"Remove all CA_descriptors with the specified CA System Id or range of ids. "
         u"Several --remove-casid options may be specified.");

    option(u"remove-pid", 0, UINT16, 0, UNLIMITED_COUNT);
    help(u"remove-pid", u"pid1[-pid2]",
         u"Remove all CA_descriptors with the specified EMM PID value or range of values. "
         u"Several --remove-pid options may be specified.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::CATPlugin::getOptions()
{
    // Get option values
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    getIntValues(_remove_casid, u"remove-casid");
    getIntValues(_remove_pid, u"remove-pid");

    // Get list of descriptors to add
    UStringVector cadescs;
    getValues(cadescs, u"add-ca-descriptor");
    _add_descs.clear();
    if (!CADescriptor::AddFromCommandLine(duck, _add_descs, cadescs)) {
        return false;
    }

    // Start superclass.
    return AbstractTablePlugin::getOptions();
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void ts::CATPlugin::createNewTable(BinaryTable& table)
{
    CAT cat;
    cat.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void ts::CATPlugin::modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all)
{
    // Warn about non-CAT tables in the CAT PID but keep them.
    if (table.tableId() != TID_CAT) {
        warning(u"found table id %n in the CAT PID", table.tableId());
        is_target = false;
        return;
    }

    // Process the CAT.
    CAT cat(duck, table);
    if (!cat.isValid()) {
        warning(u"found invalid CAT");
        reinsert = false;
        return;
    }

    // A CAT has no table id extension, but clean them all anyway.
    replace_all = true;

    // Remove descriptors
    for (size_t index = cat.descs.search(DID_MPEG_CA); index < cat.descs.count(); index = cat.descs.search(DID_MPEG_CA, index)) {
        bool remove_it = false;
        const CADescriptor desc(duck, cat.descs[index]);
        if (desc.isValid()) {
            for (size_t i = 0; !remove_it && i < _remove_casid.size(); ++i) {
                remove_it = desc.cas_id == _remove_casid[i];
            }
            for (size_t i = 0; !remove_it && i < _remove_pid.size(); ++i) {
                remove_it = desc.ca_pid == _remove_pid[i];
            }
        }
        if (remove_it) {
            cat.descs.removeByIndex(index);
        }
        else {
            index++;
        }
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        cat.descs.removeInvalidPrivateDescriptors();
    }

    // Add descriptors
    cat.descs.add(_add_descs);

    // Reserialize modified CAT.
    cat.serialize(duck, table);
}
