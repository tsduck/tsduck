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
//  Scan the NIT to get a list of tuning informations for all transports.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsSectionDemux.h"
#include "tsTunerUtils.h"
#include "tsPAT.h"
#include "tsNIT.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NITScanPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        NITScanPlugin(TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket(TSPacket&, bool&, bool&);

    private:
        UString       _output_name;     // Output file name
        std::ofstream _output_stream;   // Output file stream
        std::ostream* _output;          // Actual output stream
        UString       _comment_prefix;  // Prefix for comment lines
        UString       _variable_prefix; // Prefix for environment variable names
        bool          _use_comment;     // Add comment line
        bool          _use_variable;    // Environment variable format
        bool          _terminate;       // Terminate after one NIT
        bool          _dvb_options;     // Output format: dvb plugin options
        bool          _all_nits;        // Also include all "NIT other"
        PID           _nit_pid;         // PID for the NIT (default: read PAT)
        size_t        _nit_count;       // Number of analyzed NIT's
        SectionDemux  _demux;           // Section demux

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&);

        // Process specific tables
        void processPAT(const PAT&);
        void processNIT(const NIT&);

        // Inaccessible operations
        NITScanPlugin() = delete;
        NITScanPlugin(const NITScanPlugin&) = delete;
        NITScanPlugin& operator=(const NITScanPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(ts::NITScanPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::NITScanPlugin::NITScanPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze the NIT and output a list of tuning information.", u"[options]"),
    _output_name(),
    _output_stream(),
    _output(0),
    _comment_prefix(),
    _variable_prefix(),
    _use_comment(false),
    _use_variable(false),
    _terminate(false),
    _dvb_options(false),
    _all_nits(false),
    _nit_pid(PID_NULL),
    _nit_count(0),
    _demux(this)
{
    option(u"all-nits",    'a');
    option(u"comment",     'c', STRING, 0, 1, 0, 0, true);
    option(u"dvb-options", 'd');
    option(u"output-file", 'o', STRING);
    option(u"pid",         'p', PIDVAL);
    option(u"terminate",   't');
    option(u"variable",    'v', STRING, 0, 1, 0, 0, true);

    setHelp(u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --all-nits\n"
            u"      Analyze all NIT's (NIT actual and NIT other). By default, only the\n"
            u"      NIT actual is analyzed.\n"
            u"\n"
            u"  -c[prefix]\n"
            u"  --comment[=prefix]\n"
            u"      Add a comment line before each tuning information. The optional prefix\n"
            u"      designates the comment prefix. If the option --comment is present but the\n"
            u"      prefix is omitted, the default prefix is \"# \".\n"
            u"\n"
            u"  -d\n"
            u"  --dvb-options\n"
            u"      The characteristics of each transponder are formatted as a list of\n"
            u"      command-line options for the tsp plugin \"dvb\" such as --frequency,\n"
            u"      --symbol-rate, etc. By default, the tuning information are formatted\n"
            u"      as Linux DVB \"zap\" configuration files as used by the standard\n"
            u"      utilities \"szap\", \"czap\" and \"tzap\".\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -o filename\n"
            u"  --output-file filename\n"
            u"      Specify the output text file for the analysis result.\n"
            u"      By default, use the standard output.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Specify the PID on which the NIT is expected. By default, the PAT\n"
            u"      is analyzed to get the PID of the NIT. DVB-compliant networks should\n"
            u"      use PID 16 (0x0010) for the NIT and signal it in the PAT.\n"
            u"\n"
            u"  -t\n"
            u"  --terminate\n"
            u"      Stop the packet transmission after the first NIT is analyzed.\n"
            u"      Should be specified when tsp is used only to scan the NIT.\n"
            u"\n"
            u"  -v[prefix]\n"
            u"  --variable[=prefix]\n"
            u"      Each tuning information line is output as a shell environment variable\n"
            u"      definition. The name of each variable is built from a prefix and the TS\n"
            u"      id. The default prefix is \"TS\" and can be changed through the optional\n"
            u"      value of the option --variable.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::NITScanPlugin::start()
{
    // Get option values
    _output_name = value(u"output-file");
    _all_nits = present(u"all-nits");
    _terminate = present(u"terminate");
    _dvb_options = present(u"dvb-options");
    _nit_pid = intValue<PID>(u"pid", PID_NULL);
    _use_comment = present(u"comment");
    _comment_prefix = value(u"comment", u"# ");
    _use_variable = present(u"variable");
    _variable_prefix = value(u"variable", u"TS");

    // Initialize the demux. When the NIT PID is specified, filter this one,
    // otherwise the PAT is filtered to get the NIT PID.
    _demux.reset();
    _demux.addPID(_nit_pid != PID_NULL ? _nit_pid : PID(PID_PAT));

    // Initialize other states
    _nit_count = 0;

    // Create the output file.
    if (_output_name.empty()) {
        _output = &std::cout;
    }
    else {
        _output = &_output_stream;
        _output_stream.open(_output_name.toUTF8().c_str());
        if (!_output_stream) {
            tsp->error(u"cannot create file %s", {_output_name});
            return false;
        }
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::NITScanPlugin::stop()
{
    // Close output file
    if (!_output_name.empty()) {
        _output_stream.close();
    }

    return true;
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete table is available.
//----------------------------------------------------------------------------

void ts::NITScanPlugin::handleTable(SectionDemux& demux, const BinaryTable& table)
{
    switch (table.tableId()) {

        case TID_PAT: {
            if (table.sourcePID() == PID_PAT) {
                PAT pat(table);
                if (pat.isValid()) {
                    processPAT(pat);
                }
            }
            break;
        }

        case TID_NIT_ACT: {
            if (table.sourcePID() == _nit_pid) {
                NIT nit(table);
                if (nit.isValid()) {
                    processNIT(nit);
                }
            }
            break;
        }

        case TID_NIT_OTH: {
            if (_all_nits && table.sourcePID() == _nit_pid) {
                NIT nit(table);
                if (nit.isValid()) {
                    processNIT(nit);
                }
            }
            break;
        }

        default: {
            break;
        }
    }
}


//----------------------------------------------------------------------------
//  This method processes a Program Association Table (PAT).
//----------------------------------------------------------------------------

void ts::NITScanPlugin::processPAT(const PAT& pat)
{
    if (pat.nit_pid != PID_NULL) {
        _nit_pid = pat.nit_pid;
        tsp->verbose(u"NIT PID is %d (0x%X) in PAT", {_nit_pid, _nit_pid});
    }
    else {
        _nit_pid = PID_NIT;
        tsp->verbose(u"NIT PID not found in PAT, using default %d (0x%X)", {_nit_pid, _nit_pid});
    }

    // Filter sections on the PID for NIT.
    _demux.addPID(_nit_pid);
}


//----------------------------------------------------------------------------
//  This method processes a NIT
//----------------------------------------------------------------------------

void ts::NITScanPlugin::processNIT(const NIT& nit)
{
    tsp->debug(u"got a NIT, version %d, network Id: %d (0x%X)", {nit.version, nit.network_id, nit.network_id});

    // Count the number of NIT's
    _nit_count++;

    // Process each TS descriptor list
    for (NIT::TransportMap::const_iterator it = nit.transports.begin(); it != nit.transports.end(); ++it) {
        const TransportStreamId& tsid(it->first);
        const DescriptorList& dlist(it->second);

        // Loop on all descriptors for the current TS
        for (size_t i = 0; i < dlist.count(); ++i) {
            // Try to get delivery system information from current descriptor
            TunerParametersPtr tp(DecodeDeliveryDescriptor(*dlist[i]));
            if (!tp.isNull()) {
                // Optional comment
                if (_use_comment) {
                    *_output << _comment_prefix
                             << UString::Format(u"TS id: %d (0x%X), original network id: %d (0x%X), from NIT v%d on network id: %d (0x%X)",
                                                {tsid.transport_stream_id, tsid.transport_stream_id,
                                                 tsid.original_network_id, tsid.original_network_id,
                                                 nit.version,
                                                 nit.network_id, nit.network_id})
                             << std::endl;
                }
                // Output the tuning information, optionally in a variable definition.
                if (_use_variable) {
                    *_output << _variable_prefix << int(tsid.transport_stream_id) << "=\"";
                }
                *_output << (_dvb_options ? tp->toPluginOptions(true) : tp->toZapFormat());
                if (_use_variable) {
                    *_output << "\"";
                }
                *_output << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::NITScanPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    // Filter interesting sections
    _demux.feedPacket(pkt);

    // Exit after NIT analysis if required
    return _terminate && _nit_count > 0 ? TSP_END : TSP_OK;
}
