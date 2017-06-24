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
#include "tsFormat.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NITScanPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
    public:
        // Implementation of plugin API
        NITScanPlugin (TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual Status processPacket (TSPacket&, bool&, bool&);

    private:
        std::string   _output_name;    // Output file name
        std::ofstream _output_stream;  // Output file stream
        std::ostream* _output;         // Actual output stream
        std::string   _comment_prefix; // Prefix for comment lines
        bool          _use_comment;    // Add comment line
        bool          _terminate;      // Terminate after one NIT
        bool          _dvb_options;    // Output format: dvb plugin options
        bool          _no_nit;         // Error, no NIT found
        bool          _all_nits;       // Also include all "NIT other"
        PID           _nit_pid;        // PID for the NIT (default: read PAT)
        size_t        _nit_count;      // Number of analyzed NIT's
        SectionDemux  _demux;          // Section demux

        // Invoked by the demux when a complete table is available.
        virtual void handleTable (SectionDemux&, const BinaryTable&);

        // Process specific tables
        void processPAT (const PAT&);
        void processNIT (const NIT&);

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

ts::NITScanPlugin::NITScanPlugin (TSP* tsp_) :
    ProcessorPlugin(tsp_, "Analyze the NIT and output a list of tuning information.", "[options]"),
    _output_name(),
    _output_stream(),
    _output(0),
    _comment_prefix(),
    _use_comment(false),
    _terminate(false),
    _dvb_options(false),
    _no_nit(false),
    _all_nits(false),
    _nit_pid(PID_NIT),
    _nit_count(0),
    _demux(this)
{
    option ("all-nits",    'a');
    option ("comment",     'c', STRING, 0, 1, 0, 0, true);
    option ("dvb-options", 'd');
    option ("output-file", 'o', STRING);
    option ("pid",         'p', PIDVAL);
    option ("terminate",   't');

    setHelp ("Options:\n"
             "\n"
             "  -a\n"
             "  --all-nits\n"
             "      Analyze all NIT's (NIT actual and NIT other). By default, only the\n"
             "      NIT actual is analyzed.\n"
             "\n"
             "  -c[prefix]\n"
             "  --comment[=prefix]\n"
             "      Add a comment line before each tuning information. The optional prefix\n"
             "      designates the comment prefix. If the option --comment is present but the\n"
             "      prefix is omitted, the default prefix is \"# \".\n"
             "\n"
             "  -d\n"
             "  --dvb-options\n"
             "      The characteristics of each transponder are formatted as a list of\n"
             "      command-line options for the tsp plugin \"dvb\" such as --frequency,\n"
             "      --symbol-rate, etc. By default, the tuning information are formatted\n"
             "      as Linux DVB \"zap\" configuration files as used by the standard\n"
             "      utilities \"szap\", \"czap\" and \"tzap\".\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -o filename\n"
             "  --output-file filename\n"
             "      Specify the output text file for the analysis result.\n"
             "      By default, use the standard output.\n"
             "\n"
             "  -p value\n"
             "  --pid value\n"
             "      Specify the PID on which the NIT is expected. By default, the PAT\n"
             "      is analyzed to get the PID of the NIT. DVB-compliant networks should\n"
             "      use PID 16 (0x0010) for the NIT and signal it in the PAT.\n"
             "\n"
             "  -t\n"
             "  --terminate\n"
             "      Stop the packet transmission after the first NIT is analyzed.\n"
             "      Should be specified when tsp is used only to scan the NIT.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::NITScanPlugin::start()
{
    // Get option values
    _output_name = value ("output-file");
    _all_nits = present ("all-nits");
    _terminate = present ("terminate");
    _dvb_options = present ("dvb-options");
    _nit_pid = intValue<PID> ("pid", PID_NULL);
    _use_comment = present ("comment");
    _comment_prefix = value ("comment", "# ");

    // Initialize the demux. When the NIT PID is specified, filter this one,
    // otherwise the PAT is filtered to get the NIT PID.
    _demux.reset();
    _demux.addPID (_nit_pid != PID_NULL ? _nit_pid : PID (PID_PAT));

    // Initialize other states
    _no_nit = false;
    _nit_count = 0;

    // Create the output file.
    if (_output_name.empty()) {
        _output = &std::cout;
    }
    else {
        _output = &_output_stream;
        _output_stream.open (_output_name.c_str());
        if (!_output_stream) {
            tsp->error ("cannot create file %s", _output_name.c_str());
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
        // Extract NIT PID from PAT
        _nit_pid = pat.nit_pid;
        // Filter sections on this PID
        _demux.addPID(_nit_pid);
    }
    if (_nit_pid == PID_NULL) {
        tsp->error("NIT PID not found in PAT");
        _no_nit = true;
    }
    else {
        tsp->verbose("NIT PID is %d (0x%04X) in PAT", int (_nit_pid), int (_nit_pid));
    }
}


//----------------------------------------------------------------------------
//  This method processes a NIT
//----------------------------------------------------------------------------

void ts::NITScanPlugin::processNIT(const NIT& nit)
{
    tsp->debug("got a NIT, version %d, network Id: %d (0x%04X)", int(nit.version), int(nit.network_id), int(nit.network_id));

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
                             << Format("TS id: %d (0x%04X), original network id: %d (0x%04X), from NIT v%d on network id: %d (0x%04X)",
                                       int(tsid.transport_stream_id), int(tsid.transport_stream_id),
                                       int(tsid.original_network_id), int(tsid.original_network_id),
                                       int(nit.version),
                                       int(nit.network_id), int(nit.network_id))
                             << std::endl;
                }
                // Output the tuning information
                *_output << (_dvb_options ? tp->toPluginOptions(true) : tp->toZapFormat()) << std::endl;
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
    return _terminate && (_nit_count > 0 || _no_nit) ? TSP_END : TSP_OK;
}
