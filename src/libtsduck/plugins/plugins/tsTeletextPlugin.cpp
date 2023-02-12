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

#include "tsTeletextPlugin.h"
#include "tsTeletextFrame.h"
#include "tsTeletextDescriptor.h"
#include "tsPluginRepository.h"

TS_REGISTER_PROCESSOR_PLUGIN(u"teletext", ts::TeletextPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TeletextPlugin::TeletextPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Extract Teletext subtitles in SRT format", u"[options]"),
    _abort(false),
    _pid(PID_NULL),
    _page(-1),
    _maxFrames(0),
    _language(),
    _outFile(),
    _service(duck, this),
    _demux(duck, this, NoPID),
    _srtOutput(),
    _pages()
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"colors", 'c');
    help(u"colors",
         u"Add font color tags in the subtitles. By default, no color is specified.");

    option(u"language", 'l', STRING);
    help(u"language", u"name",
         u"Specifies the language of the subtitles to select. This option is useful "
         u"only with --service, when the PMT of the service declares Teletext "
         u"subtitles in different languages.");

    option(u"max-frames", 'm', POSITIVE);
    help(u"max-frames",
         u"Specifies the maximum number of Teletext frames to extract. The processing "
         u"is then stopped. By default, all frames are extracted.");

    option(u"output-file", 'o', FILENAME);
    help(u"output-file", u"filename",
         u"Specifies the SRT output file name. This is a text file. By default, the "
         u"SRT subtitles are displayed on the standard output.");

    option(u"page", 0, POSITIVE);
    help(u"page",
         u"Specifies the Teletext page to extract. This option is useful only when "
         u"the Teletext PID contains several pages. By default, the first Teletext "
         u"frame defines the page to use.");

    option(u"pid", 'p', PIDVAL);
    help(u"pid",
         u"Specifies the PID carrying Teletext subtitles. Alternatively, if the "
         u"Teletext PID is properly signalled in the PMT of its service, the option "
         u"--service can be used instead.");

    option(u"service", 's', STRING);
    help(u"service",
         u"Specifies the service with Teletext subtitles. If the argument is an "
         u"integer value (either decimal or hexadecimal), it is interpreted as a "
         u"service id. Otherwise, it is interpreted as a service name, as specified "
         u"in the SDT. The name is not case sensitive and blanks are ignored. "
         u"The first teletext_descriptor in the PMT of the service is used to "
         u"identify the PID carrying Teletext subtitles. If neither --service nor "
         u"--pid is specified, the first service in the PAT is used.");
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::TeletextPlugin::start()
{
    // Get command line arguments.
    duck.loadArgs(*this);
    _service.set(value(u"service"));
    _pid = intValue<PID>(u"pid", PID_NULL);
    _page = intValue<int>(u"page", -1);
    _maxFrames = intValue<int>(u"max-frames", 0);
    getValue(_language, u"language");
    getValue(_outFile, u"output-file");
    _demux.setAddColors(present(u"colors"));

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
    _pages.clear();

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
    _demux.flushTeletext();
    _srtOutput.close();
    return true;
}


//----------------------------------------------------------------------------
// Invoked by the service discovery when the PMT of the service is available.
//----------------------------------------------------------------------------

void ts::TeletextPlugin::handlePMT(const PMT& pmt, PID)
{
    bool languageOK = _language.empty();
    bool pageOK = _page < 0;

    // Analyze all components in the PMT until our Teletext PID is found.
    for (auto it = pmt.streams.begin(); _pid == PID_NULL && it != pmt.streams.end(); ++it) {
        const PID pid = it->first;
        const PMT::Stream& stream(it->second);

        // Look for Teletext descriptors for this component.
        for (size_t index = stream.descs.search(DID_TELETEXT); _pid == PID_NULL && index < stream.descs.count(); index = stream.descs.search(DID_TELETEXT, index + 1)) {
            const TeletextDescriptor desc(duck, *stream.descs[index]);
            if (_page < 0 && _language.empty()) {
                // If page and language are unspecified, keep the first Teletext PID.
                _pid = pid;
            }
            else if (desc.isValid()) {
                // Loop on all descriptor entries, until we find a matching one.
                for (auto itEntry = desc.entries.begin(); _pid == PID_NULL && itEntry != desc.entries.end(); ++itEntry) {
                    // Does it match the requested language and/or page?
                    const bool matchLanguage = _language.empty() || _language.similar(itEntry->language_code);
                    const bool matchPage = _page < 0 || _page == itEntry->page_number;
                    if (matchPage && matchLanguage) {
                        _pid = pid;
                    }
                    // Keep track of languages and pages we found.
                    languageOK = languageOK || matchLanguage;
                    pageOK = pageOK || matchPage;
                }
            }
        }
    }

    if (_pid != PID_NULL) {
        // Found a Teletext PID, demux it.
        _demux.addPID(_pid);
        tsp->verbose(u"using Teletext PID 0x%X (%d)", {_pid, _pid});
    }
    else {
        // Display error if you could not find any appropriate Teletext PID
        if (!pageOK) {
            tsp->error(u"no Teletext page %d declared in PMT", {_page});
        }
        if (!languageOK) {
            tsp->error(u"no Teletext subtitles found for language \"%s\"", {_language});
        }
        if (pageOK && languageOK) {
            tsp->error(u"no Teletext subtitles found for service 0x%X (%d)", {pmt.service_id, pmt.service_id});
        }
        _abort = true;
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
        _pages.insert(_page);
        tsp->verbose(u"using Teletext page %d", {_page});
    }

    // For information, report all Teletext pages in the PID.
    if (_pages.count(frame.page()) == 0) {
        _pages.insert(frame.page());
        tsp->verbose(u"Teletext page %d found in PID 0x%X (%d)", {frame.page(), frame.pid(), frame.pid()});
    }

    // Save only frames from the selected Teletext page.
    if (frame.page() == _page) {
        // Format frame as SRT.
        _srtOutput.addFrame(frame.showTimestamp(), frame.hideTimestamp(), frame.lines());

        // Count frames and stop when the maximum is reached.
        if (_maxFrames > 0 && frame.frameCount() >= _maxFrames) {
            _abort = true;
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::TeletextPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // As long as the Teletext PID is not found, we look for the service.
    if (_pid == PID_NULL) {
        _service.feedPacket(pkt);
    }

    // Demux Teletext streams.
    _demux.feedPacket(pkt);

    // Do not change packet but abort on error.
    return _service.nonExistentService() || _abort ? TSP_END : TSP_OK;
}
