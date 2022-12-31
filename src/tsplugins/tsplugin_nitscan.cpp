//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsPluginRepository.h"
#include "tsSectionDemux.h"
#include "tsBinaryTable.h"
#include "tsFileUtils.h"
#include "tsChannelFile.h"
#include "tsPAT.h"
#include "tsNIT.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class NITScanPlugin: public ProcessorPlugin, private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(NITScanPlugin);
    public:
        // Implementation of plugin API
        NITScanPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

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
        bool          _nit_other;       // Analyze one "NIT other"
        uint16_t      _network_id;      // Network id of "NIT other" to analyze
        PID           _nit_pid;         // PID for the NIT (default: read PAT)
        size_t        _nit_count;       // Number of analyzed NIT's
        SectionDemux  _demux;           // Section demux
        ChannelFile   _channels;        // Channel database
        UString       _channel_file;    // Name of channel configuration file.
        bool          _save_channel_file;     // Save a fresh new version of channel configuration file.
        bool          _update_channel_file;   // Update previous content of channel configuration file.
        bool          _default_channel_file;  // Use default channel configuration file.

        // Invoked by the demux when a complete table is available.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;

        // Process specific tables
        void processPAT(const PAT&);
        void processNIT(const NIT&);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"nitscan", ts::NITScanPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::NITScanPlugin::NITScanPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Analyze the NIT and output a list of tuning information", u"[options]"),
    _output_name(),
    _output_stream(),
    _output(nullptr),
    _comment_prefix(),
    _variable_prefix(),
    _use_comment(false),
    _use_variable(false),
    _terminate(false),
    _dvb_options(false),
    _all_nits(false),
    _nit_other(false),
    _network_id(0),
    _nit_pid(PID_NULL),
    _nit_count(0),
    _demux(duck, this),
    _channels(),
    _channel_file(),
    _save_channel_file(false),
    _update_channel_file(false),
    _default_channel_file(false)
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"all-nits", 'a');
    help(u"all-nits",
         u"Analyze all NIT's (NIT actual and NIT other). By default, only the "
         u"NIT actual is analyzed.");

    option(u"comment", 'c', STRING, 0, 1, 0, 0, true);
    help(u"comment", u"prefix",
         u"Add a comment line before each tuning information. The optional prefix "
         u"designates the comment prefix. If the option --comment is present but the "
         u"prefix is omitted, the default prefix is \"# \".");

    option(u"dvb-options", 'd');
    help(u"dvb-options",
         u"The characteristics of each transponder are formatted as a list of "
         u"command-line options for the tsp plugin \"dvb\" such as --frequency, "
         u"--symbol-rate, etc. This is the default when no --save-channels or "
         u"--update-channels is specified.");

    option(u"network-id", 'n', UINT16);
    help(u"network-id",
         u"Specify the network-id of a NIT other to analyze instead of the NIT actual. "
         u"By default, the NIT actual is analyzed.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specify the output text file for the analysis result. "
         u"By default, use the standard output.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specify the PID on which the NIT is expected. By default, the PAT "
         u"is analyzed to get the PID of the NIT. DVB-compliant networks should "
         u"use PID 16 (0x0010) for the NIT and signal it in the PAT.");

    option(u"save-channels", 0, FILENAME);
    help(u"save-channels", u"filename",
         u"Save the description of all transport streams in the specified XML file. "
         u"If the file name is \"-\", use the default tuning configuration file. "
         u"See also option --update-channels.");

    option(u"terminate", 't');
    help(u"terminate",
         u"Stop the packet transmission after the first NIT is analyzed. "
         u"Should be specified when tsp is used only to scan the NIT.");

    option(u"update-channels", 0, FILENAME);
    help(u"update-channels", u"filename",
         u"Update the description of all transport streams in the specified XML file. "
         u"The content of each transport stream is preserved, only the tuning information is updated. "
         u"If the file does not exist, it is created. "
         u"If the file name is \"-\", use the default tuning configuration file. "
         u"See also option --save-channels.");

    option(u"variable", 'v', STRING, 0, 1, 0, 0, true);
    help(u"variable", u"prefix",
         u"Each tuning information line is output as a shell environment variable "
         u"definition. The name of each variable is built from a prefix and the TS "
         u"id. The default prefix is \"TS\" and can be changed through the optional "
         u"value of the option --variable. ");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::NITScanPlugin::getOptions()
{
    // Get option values
    duck.loadArgs(*this);
    _output_name = value(u"output-file");
    _all_nits = present(u"all-nits");
    _terminate = present(u"terminate");
    _dvb_options = present(u"dvb-options");
    _nit_other = present(u"network-id");
    _network_id = intValue<uint16_t>(u"network-id");
    _nit_pid = intValue<PID>(u"pid", PID_NULL);
    _use_comment = present(u"comment");
    _comment_prefix = value(u"comment", u"# ");
    _use_variable = present(u"variable");
    _variable_prefix = value(u"variable", u"TS");

    _save_channel_file = present(u"save-channels");
    _update_channel_file = present(u"update-channels");
    _channel_file = _update_channel_file ? value(u"update-channels") : value(u"save-channels");
    _default_channel_file = (_save_channel_file || _update_channel_file) && (_channel_file.empty() || _channel_file == u"-");

    if (_save_channel_file && _update_channel_file) {
        tsp->error(u"--save-channels and --update-channels are mutually exclusive");
        return false;
    }
    else if (_default_channel_file) {
        // Use default channel file.
        _channel_file = ChannelFile::DefaultFileName();
    }

    // Default is --dvb-options.
    _dvb_options = _dvb_options || (!_save_channel_file && !_update_channel_file);

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::NITScanPlugin::start()
{
    // Pre-load the existing channel file.
    _channels.clear();
    if (_update_channel_file && !_channel_file.empty() && FileExists(_channel_file) && !_channels.load(_channel_file, *tsp)) {
        return false;
    }

    // Initialize the demux. When the NIT PID is specified, filter this one,
    // otherwise the PAT is filtered to get the NIT PID.
    _demux.reset();
    _demux.addPID(_nit_pid != PID_NULL ? _nit_pid : PID(PID_PAT));

    // Initialize other states
    _nit_count = 0;

    // Create the output file for --dvb-options.
    if (_output_name.empty() || !_dvb_options) {
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

    // Save channels file. Create intermediate directories when it is the default file.
    if (!_channel_file.empty()) {
        tsp->verbose(u"saving %s", {_channel_file});
        _channels.save(_channel_file, _default_channel_file, *tsp);
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
                const PAT pat(duck, table);
                if (pat.isValid()) {
                    processPAT(pat);
                }
            }
            break;
        }

        case TID_NIT_ACT: {
            if (table.sourcePID() == _nit_pid) {
                const NIT nit(duck, table);
                if (nit.isValid()) {
                    processNIT(nit);
                }
            }
            break;
        }

        case TID_NIT_OTH: {
            if (table.sourcePID() == _nit_pid) {
                const NIT nit(duck, table);
                if (nit.isValid() && (_all_nits || (_nit_other && _network_id == nit.network_id))) {
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
    for (const auto& it : nit.transports) {
        const TransportStreamId& tsid(it.first);
        const DescriptorList& dlist(it.second.descs);

        // Loop on all descriptors for the current TS
        for (size_t i = 0; i < dlist.count(); ++i) {
            // Try to get delivery system information from current descriptor
            ModulationArgs tp;
            if (tp.fromDeliveryDescriptor(duck, *dlist[i], tsid.transport_stream_id)) {

                // Output --dvb-options.
                if (_dvb_options) {
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
                    *_output << tp.toPluginOptions(true);
                    if (_use_variable) {
                        *_output << "\"";
                    }
                    *_output << std::endl;
                }

                // Fill the channel database.
                if (_save_channel_file || _update_channel_file) {
                    // Get or create network description in channel database.
                    // Use tuner type from delivery descriptor.
                    ChannelFile::NetworkPtr net(_channels.networkGetOrCreate(nit.network_id, TunerTypeOf(tp.delivery_system.value(DS_UNDEFINED))));
                    // Get or create TS description in channel database.
                    ChannelFile::TransportStreamPtr ts(net->tsGetOrCreate(tsid.transport_stream_id));
                    // Do not reset services in TS, keep existing if any, just update tuning info.
                    ts->onid = tsid.original_network_id;
                    ts->tune = tp;
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::NITScanPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // Filter interesting sections
    _demux.feedPacket(pkt);

    // Exit after NIT analysis if required
    return _terminate && _nit_count > 0 ? TSP_END : TSP_OK;
}
