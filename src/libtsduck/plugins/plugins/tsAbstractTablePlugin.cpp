//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractTablePlugin.h"


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::AbstractTablePlugin::AbstractTablePlugin(TSP* tsp_,
                                             const UString& description,
                                             const UString& syntax,
                                             const UString& table_name,
                                             PID pid,
                                             const BitRate& default_bitrate,
                                             const UString& new_table_help) :
    ProcessorPlugin(tsp_, description, syntax),
    _table_name(table_name),
    _default_bitrate(default_bitrate),
    _pid(pid),
    _demux(duck, this),
    _pzer(duck, pid),
    _patch_xml(duck)
{
    _patch_xml.defineArgs(*this);

    option<BitRate>(u"bitrate", 'b');
    help(u"bitrate",
         u"Specifies the bitrate in bits / second of the " + _table_name + " PID if a new one is "
         u"created. The default is " + _default_bitrate.toString() + u" b/s.");

    option(u"create", 'c');
    help(u"create",
         u"Create a new empty " + _table_name + u" if none was received after one second. This is "
         u"equivalent to --create-after 1000.");

    option<cn::milliseconds>(u"create-after");
    help(u"create-after",
         u"Create a new empty " + _table_name + u" if none was received after the specified number "
         u"of milliseconds. If the actual " + _table_name + u" is received later, it will be used "
         u"as the base for transformations instead of the empty one." +
         UString(new_table_help.empty() ? u"" : u"\n\n") + new_table_help);

    option(u"increment-version", 'i');
    help(u"increment-version",
         u"Increment the version number of the " + _table_name + u".");

    option(u"inter-packet", 0, POSITIVE);
    help(u"inter-packet",
         u"When a new " + _table_name + u" is created and --bitrate is not present, this option "
         u"specifies the packet interval for the PID, that is to say the number of TS packets in "
         u"the transport between two packets of the PID. Use instead of --bitrate if the global "
         u"bitrate of the TS cannot be determined.");

    option(u"new-version", 'v', INTEGER, 0, 1, 0, 31);
    help(u"new-version",
         u"Specify a new value for the version of the " + _table_name + u".");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::AbstractTablePlugin::getOptions()
{
    _set_version = present(u"new-version");
    _incr_version = present(u"increment-version");
    getChronoValue(_create_after_ms, u"create-after", cn::seconds(present(u"create") ? 1 : 0));
    getValue(_bitrate, u"bitrate", _default_bitrate);
    getIntValue(_inter_pkt, u"inter-packet", 0);
    getIntValue(_new_version, u"new-version", 0);
    bool ok = _patch_xml.loadArgs(duck, *this);

    if (present(u"create") && present(u"create-after")) {
        error(u"options --create and --create-after are mutually exclusive");
        ok = false;
    }

    return ok;
}


//----------------------------------------------------------------------------
// Set a new PID to process.
//----------------------------------------------------------------------------

void ts::AbstractTablePlugin::setPID(PID pid)
{
    // Reset demux and packetizer if we change PID.
    if (pid != _pid) {
        _pid = pid;
        _found_pid = false;
        _demux.reset();
        _demux.addPID(_pid);
        _pzer.reset();
        _pzer.setPID(_pid);
    }
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::AbstractTablePlugin::start()
{
    // Initialize the demux and packetizer
    _demux.reset();
    _demux.addPID(_pid);
    _pzer.reset();
    _pzer.setPID(_pid);

    // Reset other states
    _found_pid = _found_table = false;
    _pkt_create = _pkt_insert = tsp->pluginPackets();

    // Load XML patch files.
    return _patch_xml.loadPatchFiles();
}


//----------------------------------------------------------------------------
// Invoked by the demux when a table is found on the input PID.
//----------------------------------------------------------------------------

void ts::AbstractTablePlugin::handleTable(SectionDemux& demux, const BinaryTable& intable)
{
    // Filter out call for some other demux (maybe from a suclass).
    if (&demux != &_demux) {
        return;
    }

    // Save table characteritics.
    const bool is_short = intable.isShortSection();
    const TID initial_tid = intable.tableId();
    const uint16_t initial_tidext = intable.tableIdExtension();

    // Build a modifiable version of the table.
    BinaryTable table(intable, ShareMode::SHARE);

    // Process XML patching.
    if (!_patch_xml.applyPatches(table)) {
        return; // error displayed in applyPatches()
    }

    // If the patch file deleted the table, remove it from the packetizer.
    if (!table.isValid()) {
        if (is_short) {
            _pzer.removeSections(initial_tid);
        }
        else {
            _pzer.removeSections(initial_tid, initial_tidext);
        }
        return;
    }

    // Call subclass to process the table.
    bool is_target = true;
    bool reinsert = true;
    bool replace_all = false;
    modifyTable(table, is_target, reinsert, replace_all);

    // Place modified table in the packetizer.
    if (reinsert) {
        reinsertTable(table, initial_tid, initial_tidext, is_target, replace_all);
    }
}


//----------------------------------------------------------------------------
// Called by the subclass when some external event forces an update of the table.
//----------------------------------------------------------------------------

void ts::AbstractTablePlugin::forceTableUpdate(BinaryTable& table, bool replace_all)
{
    // Common processing of target table.
    reinsertTable(table, table.tableId(), table.tableIdExtension(), true, replace_all);

    // Insert first packet as soon as possible when the target PID is not present.
    _pkt_insert = tsp->pluginPackets();
}


//----------------------------------------------------------------------------
// Reinsert a table in the target PID.
//----------------------------------------------------------------------------

void ts::AbstractTablePlugin::reinsertTable(BinaryTable& table, TID initial_tid, uint16_t initial_tidext, bool is_target_table, bool replace_all)
{
    // Make common modifications on target table.
    if (is_target_table) {
        verbose(u"%s version %d modified", _table_name, table.version());

        // The target table is found, no longer need to create a new one.
        _found_table = true;
        _pkt_insert = 0;

        // Modify the table version.
        if (_incr_version) {
            table.setVersion((table.version() + 1) & SVERSION_MASK);
        }
        else if (_set_version) {
            table.setVersion(_new_version);
        }
    }

    // Remove previous instances of the table.
    if (table.isShortSection() || replace_all) {
        _pzer.removeSections(initial_tid);
    }
    else {
        _pzer.removeSections(initial_tid, initial_tidext);
    }

    // Reinsert the table in the packetizer.
    _pzer.addTable(table);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::AbstractTablePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();
    if (pid == _pid) {
        _found_pid = true;
    }

    // Filter incoming sections
    _demux.feedPacket(pkt);

    // Determine when a new table shall be created. Executed only once, when the bitrate is known
    if (!_found_table && _create_after_ms > cn::milliseconds::zero() && _pkt_create == 0) {
        const BitRate ts_bitrate = tsp->bitrate();
        if (ts_bitrate > 0) {
            _pkt_create = PacketDistance(ts_bitrate, _create_after_ms);
            debug(u"will create %s after %'d packets, %'!s (bitrate: %'d b/s)", _table_name, _pkt_create, _create_after_ms, ts_bitrate);
        }
    }

    // Create a new table when necessary.
    if (!_found_table && _pkt_create > 0 && tsp->pluginPackets() >= _pkt_create) {
        // Let the subclass create a new empty table.
        BinaryTable table;
        createNewTable(table);
        // Now pretend to have collected the table from the stream
        // so that the subclass can apply its modifications.
        handleTable(_demux, table);
        // Insert first packet as soon as possible when the target PID is not present.
        _pkt_insert = tsp->pluginPackets();
    }

    // Insertion of packets from the input PID.
    if (!_found_pid && pid == PID_NULL && _pkt_insert > 0 && tsp->pluginPackets() >= _pkt_insert) {
        // It is time to replace stuffing by a created table packet.
        _pzer.getNextPacket(pkt);
        // Next insertion point.
        if (_inter_pkt != 0) {
            // Packet interval was explicitly specified for the created PID.
            _pkt_insert += _inter_pkt;
        }
        else {
            // Compute packet interval from bitrates.
            const BitRate ts_bitrate = tsp->bitrate();
            if (ts_bitrate < _bitrate) {
                error(u"input bitrate unknown or too low, specify --inter-packet instead of --bitrate");
                return TSP_END;
            }
            _pkt_insert += (ts_bitrate / _bitrate).toInt();
        }
    }
    else if (pid == _pid) {
        // Replace an existing input PID packet.
        _pzer.getNextPacket(pkt);
    }

    return _abort ? TSP_END : TSP_OK;
}
