//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Various transformations on the CAT.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsCyclingPacketizer.h"
#include "tsCADescriptor.h"
#include "tsDecimal.h"
#include "tsCAT.h"
TSDUCK_SOURCE;

#define DEFAULT_CAT_BITRATE 3000


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class CATPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        CATPlugin (TSP*);
        virtual bool start();
        virtual bool stop() {return true;}
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        bool                  _cat_found;         // Found a CAT
        PacketCounter         _pkt_current;       // Total number of packets
        PacketCounter         _pkt_create_cat;    // Packet# after which a new CAT shall be created
        PacketCounter         _pkt_insert_cat;    // Packet# after which a CAT packet shall be inserted
        MilliSecond           _create_after_ms;   // Create a new CAT if none found after that time
        BitRate               _cat_bitrate;       // CAT PID's bitrate (if no previous CAT)
        PacketCounter         _cat_inter_pkt;     // Packet interval between two CAT packets
        bool                  _cleanup_priv_desc; // Remove private desc without preceding PDS desc
        bool                  _incr_version;      // Increment table version
        bool                  _set_version;       // Set a new table version
        uint8_t               _new_version;       // New table version
        std::vector<uint16_t> _remove_casid;      // Set of CAS id to remove
        std::vector<uint16_t> _remove_pid;        // Set of EMM PID to remove
        DescriptorList        _add_descs;         // List of descriptors to add
        SectionDemux          _demux;             // Section demux
        CyclingPacketizer     _pzer;              // Packetizer for modified CAT

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);
        void handleCAT (CAT&);

        // Inaccessible operations
        CATPlugin() = delete;
        CATPlugin(const CATPlugin&) = delete;
        CATPlugin& operator=(const CATPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::CATPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::CATPlugin::CATPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Perform various transformations on the CAT", u"[options]"),
    _cat_found(false),
    _pkt_current(0),
    _pkt_create_cat(0),
    _pkt_insert_cat(0),
    _create_after_ms(0),
    _cat_bitrate(0),
    _cat_inter_pkt(0),
    _cleanup_priv_desc(false),
    _incr_version(false),
    _set_version(false),
    _new_version(0),
    _remove_casid(),
    _remove_pid(),
    _add_descs(),
    _demux(this),
    _pzer()
{
    option(u"add",                        'a', STRING, 0, UNLIMITED_COUNT);
    option(u"bitrate",                    'b', POSITIVE);
    option(u"cleanup-private-descriptors", 0);
    option(u"create",                     'c');
    option(u"create-after",                0,  POSITIVE);
    option(u"increment-version",          'i');
    option(u"inter-packet",                0,  POSITIVE);
    option(u"remove-casid",               'r', UINT16, 0, UNLIMITED_COUNT);
    option(u"remove-pid",                  0,  UINT16, 0, UNLIMITED_COUNT);
    option(u"new-version",                'v', INTEGER, 0, 1, 0, 31);

    setHelp(u"Options:\n"
            u"\n"
            u"  -a casid/pid[/private-data]\n"
            u"  --add casid/pid[/private-data]\n"
            u"      Add a CA_descriptor in the CAT with the specified CA System Id and\n"
            u"      EMM PID. The optional private data must be a suite of hexadecimal digits.\n"
            u"      Several --add options may be specified to add several descriptors.\n"
            u"\n"
            u"  -b value\n"
            u"  --bitrate value\n"
            u"      Specifies the bitrate in bits / second of the CAT if a new one is\n"
            u"      created. The default is " + Decimal (DEFAULT_CAT_BITRATE) + " b/s.\n"
            u"\n"
            u"  --cleanup-private-descriptors\n"
            u"      Remove all private descriptors without preceding private_data_specifier\n"
            u"      descriptor.\n"
            u"\n"
            u"  -c\n"
            u"  --create\n"
            u"      Create a new empty CAT if none was received after one second. This is\n"
            u"      equivalent to --create-after 1000.\n"
            u"\n"
            u"  --create-after milliseconds\n"
            u"      Create a new empty CAT if none was received after the specified number\n"
            u"      of milliseconds. This can be useful to force the creation of a CAT in\n"
            u"      a TS that has none (the CAT is an optional table). If an actual CAT is\n"
            u"      received later, it will be used as the base for transformations instead\n"
            u"      of the empty one.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -i\n"
            u"  --increment-version\n"
            u"      Increment the version number of the CAT.\n"
            u"\n"
            u"  --inter-packet value\n"
            u"      When a new CAT is created and --bitrate is not present, this option\n"
            u"      specifies the packet interval for the CAT PID, that is to say the\n"
            u"      number of TS packets in the transport between two packets of the\n"
            u"      CAT PID. Use instead of --bitrate if the global bitrate of the TS\n"
            u"      cannot be determined.\n"
            u"\n"
            u"  -v value\n"
            u"  --new-version value\n"
            u"      Specify a new value for the version of the CAT.\n"
            u"\n"
            u"  -r value\n"
            u"  --remove-casid value\n"
            u"      Remove all CA_descriptors with the specified CA System Id.\n"
            u"      Several --remove-casid options may be specified.\n"
            u"\n"
            u"  --remove-pid value\n"
            u"      Remove all CA_descriptors with the specified EMM PID value.\n"
            u"      Several --remove-pid options may be specified.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::CATPlugin::start()
{
    // Get option values
    _incr_version = present(u"increment-version");
    _create_after_ms = present(u"create") ? 1000 : intValue<MilliSecond>(u"create-after", 0);
    _cat_bitrate = intValue<BitRate>(u"bitrate", DEFAULT_CAT_BITRATE);
    _cat_inter_pkt = intValue<PacketCounter>(u"inter-packet", 0);
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    _set_version = present(u"new-version");
    _new_version = intValue<uint8_t>(u"new-version", 0);
    getIntValues (_remove_casid, "remove-casid");
    getIntValues (_remove_pid, "remove-pid");

    // Get list of descriptors to add
    const size_t add_count = count ("add");
    _add_descs.clear();
    for (size_t n = 0; n < add_count; n++) {
        const std::string val (value(u"add", "", n));
        int casid, pid;
        char slash;
        const int count = ::sscanf (val.c_str(), "%i/%i%c", &casid, &pid, &slash);
        if ((count != 2 && (count != 3 || slash != '/')) || casid < 0 || casid > 0xFFFF || pid < 0 || pid >= PID_MAX) {
            tsp->error ("invalid \"cas-id/PID[/private-data]\" value \"" + val + "\"");
            return false;
        }
        CADescriptor desc;
        desc.cas_id = uint16_t (casid);
        desc.ca_pid = PID (pid);
        if (count == 3) {
            // There is a private part
            assert (slash == '/');
            size_t pos = val.find ('/'); // First slash
            assert (pos != std::string::npos);
            assert (pos < val.length() - 1);
            pos = val.find ('/', pos + 1); // Second slash
            assert (pos != std::string::npos);
            const std::string hexa (val.substr (pos + 1));
            if (!HexaDecode (desc.private_data, hexa)) {
                tsp->error ("invalid private data \"" + hexa + "\" for CA_descriptor, specify an even number of hexa digits");
                return false;
            }
        }
        _add_descs.add (desc);
    }

    // Initialize the demux and packetizer
    _demux.reset();
    _demux.addPID (PID_CAT);
    _pzer.reset();
    _pzer.setPID (PID_CAT);

    // Reset other states
    _cat_found = false;
    _pkt_current = 0;
    _pkt_create_cat = 0;
    _pkt_insert_cat = 0;

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::CATPlugin::handleTable (SectionDemux& demux, const BinaryTable& table)
{
    if (table.tableId() == TID_CAT && table.sourcePID() == PID_CAT) {
        CAT cat (table);
        if (cat.isValid()) {
            // Process this CAT
            handleCAT (cat);
            // No longer try to insert new CAT packets
            _pkt_insert_cat = 0;
        }
    }
}


//----------------------------------------------------------------------------
// Process a new CAT
//----------------------------------------------------------------------------

void ts::CATPlugin::handleCAT (CAT& cat)
{
    // CAT is found, no longer try to create a new one
    _cat_found = true;

    // Modify CAT version
    if (_incr_version) {
        cat.version = (cat.version + 1) & 0x1F;
    }
    else if (_set_version) {
        cat.version = _new_version;
    }

    // Remove descriptors
    for (size_t index = cat.descs.search (DID_CA); index < cat.descs.count(); index = cat.descs.search (DID_CA, index)) {
        bool remove_it = false;
        const CADescriptor desc (*(cat.descs[index]));
        if (desc.isValid()) {
            for (size_t i = 0; !remove_it && i < _remove_casid.size(); ++i) {
                remove_it = desc.cas_id == _remove_casid[i];
            }
            for (size_t i = 0; !remove_it && i < _remove_pid.size(); ++i) {
                remove_it = desc.ca_pid == _remove_pid[i];
            }
        }
        if (remove_it) {
            cat.descs.removeByIndex (index);
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
    cat.descs.add (_add_descs);

    // Place modified CAT in the packetizer
    tsp->verbose ("CAT version %d modified", int (cat.version));
    _pzer.removeSections (TID_CAT);
    _pzer.addTable (cat);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::CATPlugin::processPacket (TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    const PID pid = pkt.getPID();

    // Count packets
    _pkt_current++;

    // Filter incoming sections
    _demux.feedPacket (pkt);

    // Determine when a new CAT shall be created. Executed only once, when the bitrate is known
    if (_create_after_ms > 0 && _pkt_create_cat == 0) {
        _pkt_create_cat = PacketDistance (tsp->bitrate(), _create_after_ms);
    }

    // Create a new CAT when necessary
    if (!_cat_found && _pkt_create_cat > 0 && _pkt_current >= _pkt_create_cat) {
        // Create a new empty CAT and process it as if it comes from the TS
        CAT cat;
        handleCAT (cat);
        // Insert first CAT packet as soon as possible
        _pkt_insert_cat = _pkt_current;
    }

    // Insertion of CAT packets
    if (pid == PID_NULL && _pkt_insert_cat > 0 && _pkt_current >= _pkt_insert_cat) {
        // It is time to replace stuffing by a created CAT packet
        _pzer.getNextPacket (pkt);
        // Next CAT insertion point
        if (_cat_inter_pkt != 0) {
            // CAT packet interval was explicitely specified
            _pkt_insert_cat += _cat_inter_pkt;
        }
        else {
            // Compute CAT packet interval from bitrates
            const BitRate ts_bitrate = tsp->bitrate();
            if (ts_bitrate < _cat_bitrate) {
                tsp->error ("input bitrate unknown or too low, specify --inter-packet instead of --bitrate");
                return TSP_END;
            }
            _pkt_insert_cat += ts_bitrate / _cat_bitrate;
        }
    }
    else if (pid == PID_CAT) {
        // Replace an existing CAT packet
        _pzer.getNextPacket (pkt);
    }

    return TSP_OK;
}
