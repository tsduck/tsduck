//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Extract Teletext subtitles.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsPluginRepository.h"
#include "tsServiceDiscovery.h"
#include "tsSubRipGenerator.h"
#include "tsTeletextDemux.h"
#include "tsTeletextFrame.h"
#include "tsTeletextDescriptor.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TeletextPlugin:
        public ProcessorPlugin,
        private PMTHandlerInterface,
        private TeletextHandlerInterface
    {
    public:
        // Implementation of plugin API
        TeletextPlugin(TSP*);
        virtual bool start() override;
        virtual bool stop() override;
        virtual Status processPacket(TSPacket&, bool&, bool&) override;

    private:
        bool             _abort;      // Error (service not found, etc).
        PID              _pid;        // Teletext PID.
        int              _page;       // Teletext page.
        UString          _language;   // Language to select.
        UString          _outFile;    // Output file name.
        ServiceDiscovery _service;    // Service name & id.
        TeletextDemux    _demux;      // Teletext demux to extract subtitle frames.
        SubRipGenerator  _srtOutput;  // Generate SRT output file.

        // Implementation of interfaces.
        virtual void handlePMT(const PMT& table) override;
        virtual void handleTeletextMessage(TeletextDemux& demux, const TeletextFrame& frame) override;

        // Inaccessible operations
        TeletextPlugin() = delete;
        TeletextPlugin(const TeletextPlugin&) = delete;
        TeletextPlugin& operator=(const TeletextPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_PROCESSOR(teletext, ts::TeletextPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TeletextPlugin::TeletextPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract Teletext subtitles in SRT format.", u"[options]"),
    _abort(false),
    _pid(PID_NULL),
    _page(-1),
    _language(),
    _outFile(),
    _service(this, *tsp),
    _demux(this),
    _srtOutput()
{
    option(u"language",    'l', STRING);
    option(u"output-file", 'o', STRING);
    option(u"page",         0,  POSITIVE);
    option(u"pid",         'p', PIDVAL);
    option(u"service",     's', STRING);

    setHelp(u"Options:\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -l name\n"
            u"  --language name\n"
            u"      Specifies the language of the subtitles to select. This option is useful\n"
            u"      only with --service, when the PMT of the service declares Teletext\n"
            u"      subtitles in different languages.\n"
            u"\n"
            u"  -o filename\n"
            u"  --output-file filename\n"
            u"      Specifies the SRT output file name. This is a text file. By default, the\n"
            u"      SRT subtitles are displayed on the standard output.\n"
            u"\n"
            u"  --page value\n"
            u"      Specificies the Teletext page to extract. This option is useful only when\n"
            u"      the Teletext PID contains several pages. By default, the first Teletext\n"
            u"      frame defines the page to use.\n"
            u"\n"
            u"  -p value\n"
            u"  --pid value\n"
            u"      Specifies the PID carrying Teletext subtitles. Alternatively, if the\n"
            u"      Teletext PID is properly signaled in the PMT of its service, the option\n"
            u"      --service can be used. Exactly one of --service or --pid shall be specified.\n"
            u"\n"
            u"  -s value\n"
            u"  --service value\n"
            u"      Specifies the service with Teletext subtitles. If the argument is an\n"
            u"      integer value (either decimal or hexadecimal), it is interpreted as a\n"
            u"      service id. Otherwise, it is interpreted as a service name, as specified\n"
            u"      in the SDT. The name is not case sensitive and blanks are ignored.\n"
            u"      The first teletext_descriptor in the PMT of the service is used to\n"
            u"      identify the PID carrying Teletext subtitles.\n"
            u"      Exactly one of --service or --pid shall be specified.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TeletextPlugin::start()
{
    // Get command line arguments.
    if (present(u"pid") + present(u"service") != 1) {
        tsp->error(u"specify exactly one of --service or --pid");
        return false;
    }
    _service.set(value(u"service"));
    getValue(_language, u"language");
    getValue(_outFile, u"output-file");
    _page = intValue<int>(u"page", -1);
    _pid = intValue<PID>(u"pid", PID_NULL);

    // Create the output file.
    if (_outFile.empty()) {
        // No output file specified, use standard output.
        _srtOutput.setStream(&std::cout);
    }
    else if (!_srtOutput.open(_outFile, *tsp)) {
        // Output file creation error.
        return false;
    }

    // Reinitialize the plugin state.
    _abort = false;
    _demux.reset();

    // If the Teletext page is already known, filter it immediately.
    if (_pid != PID_NULL) {
        _demux.addPID(_pid);
    }

    return true;
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::TeletextPlugin::stop()
{
    _srtOutput.close();
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the service discovery when the PMT of the service is available.
//----------------------------------------------------------------------------

void ts::TeletextPlugin::handlePMT(const PMT& pmt)
{
    // Analyze all components in the PMT until our Teletext PID is found.
    for (PMT::StreamMap::const_iterator it = pmt.streams.begin(); _pid == PID_NULL && it != pmt.streams.end(); ++it) {
        const PID pid = it->first;
        const PMT::Stream& stream(it->second);

        // Look for Teletext descriptors for this component.
        for (size_t index = stream.descs.search(DID_TELETEXT); _pid == PID_NULL && index < stream.descs.count(); index = stream.descs.search(DID_TELETEXT, index + 1)) {
            const TeletextDescriptor desc(*stream.descs[index]);
            if (_page < 0 && _language.empty()) {
                // If page and language are unspecified, keep the first Teletext PID.
                _pid = pid;
            }
            else if (desc.isValid()) {
                // Loop on all descriptor entries, until we find a matching one.
                for (TeletextDescriptor::EntryList::const_iterator itEntry = desc.entries.begin(); _pid == PID_NULL && itEntry != desc.entries.end(); ++itEntry) {
                    const bool matchLanguage = _language.empty() || _language.similar(itEntry->language_code);
                    const bool matchPage = _page < 0 || _page == itEntry->page_number;
                    if (matchPage && matchLanguage) {
                        _pid = pid;
                    }
                }
            }
        }
    }
}


//----------------------------------------------------------------------------
// Invoked when a complete Teletext message is available.
//----------------------------------------------------------------------------

void ts::TeletextPlugin::handleTeletextMessage(TeletextDemux& demux, const TeletextFrame& frame)
{
    // If the Teletext page was not specified, use the first one.
    if (_page < 0) {
        _page = frame.page();
    }

    // Save only frames from the selected Teletext page.
    if (frame.page() == _page) {
        _srtOutput.addFrame(frame.showTimestamp(), frame.hideTimestamp(), frame.lines());
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TeletextPlugin::processPacket(TSPacket& pkt, bool& flush, bool& bitrate_changed)
{
    _service.feedPacket(pkt);
    _demux.feedPacket(pkt);
    return _service.nonExistentService() || _abort ? TSP_END : TSP_OK;
}
