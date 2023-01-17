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
//  Merge PSI/SI from mixed streams
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsPSIMerger.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PSIMergePlugin: public ProcessorPlugin
    {
        TS_NOBUILD_NOCOPY(PSIMergePlugin);
    public:
        // Implementation of plugin API
        PSIMergePlugin (TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        PSIMerger _psi_merger;    // Engine to merge PSI/SI.
        size_t    _main_label;    // Label of packets from main stream or greater than LABEL_MAX if none.
        size_t    _merge_label;   // Label of packets from main stream or greater than LABEL_MAX if none.
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"psimerge", ts::PSIMergePlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PSIMergePlugin::PSIMergePlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Merge PSI/SI from mixed streams", u"[options]"),
    _psi_merger(duck, PSIMerger::NONE),
    _main_label(TSPacketLabelSet::MAX + 1),
    _merge_label(TSPacketLabelSet::MAX + 1)
{
    setIntro(u"This plugin assumes that the PSI/SI for two independent streams "
             u"are multiplexed in the same transport streams but the packets from "
             u"each original stream are independently labelled. This plugin merges "
             u"the PSI/SI from these two streams into one.");

    option(u"no-cat");
    help(u"no-cat", u"Do not merge the CAT.");

    option(u"no-pat");
    help(u"no-pat", u"Do not merge the PAT.");

    option(u"no-eit");
    help(u"no-eit", u"Do not merge the EIT's.");

    option(u"no-sdt");
    help(u"no-sdt", u"Do not merge the SDT Actual.");

    option(u"no-nit");
    help(u"no-nit", u"Do not merge the NIT Actual.");

    option(u"no-bat");
    help(u"no-bat", u"Do not merge the BAT.");

    option(u"time-from-merge");
    help(u"time-from-merge",
         u"Use the TDT/TOT time reference from the 'merge' stream. "
         u"By default, use the TDT/TOT time reference from the 'main' stream.");

    option(u"main-label", 0, INTEGER, 0, 1, 0, TSPacketLabelSet::MAX);
    help(u"main-label",
        u"Specify the label which is set on packets from the 'main' stream. "
        u"The maximum label value is " + UString::Decimal(TSPacketLabelSet::MAX) + u". "
        u"By default, the main stream is made of packets without label. "
        u"At least one of --main-label and --merge-label must be specified.");

    option(u"merge-label", 0, INTEGER, 0, 1, 0, TSPacketLabelSet::MAX);
    help(u"merge-label",
        u"Specify the label which is set on packets from the 'merge' stream. "
        u"The maximum label value is " + UString::Decimal(TSPacketLabelSet::MAX) + u". "
        u"By default, the merge stream is made of packets without label. "
        u"At least one of --main-label and --merge-label must be specified.");
}


//----------------------------------------------------------------------------
// Get options method
//----------------------------------------------------------------------------

bool ts::PSIMergePlugin::getOptions()
{
    // Identification of main and merge streams.
    _main_label = intValue<size_t>(u"main-label", TSPacketLabelSet::MAX + 1);
    _merge_label = intValue<size_t>(u"merge-label", TSPacketLabelSet::MAX + 1);
    if (_main_label == _merge_label) {
        tsp->error(u"at least one of --main-label and --merge-label must be specified and the labels must be different");
        return false;
    }

    // Build PSI merger options.
    PSIMerger::Options options = PSIMerger::NULL_MERGED;
    if (!present(u"no-cat")) {
        options |= PSIMerger::MERGE_CAT;
    }
    if (!present(u"no-pat")) {
        options |= PSIMerger::MERGE_PAT;
    }
    if (!present(u"no-sdt")) {
        options |= PSIMerger::MERGE_SDT;
    }
    if (!present(u"no-nit")) {
        options |= PSIMerger::MERGE_NIT;
    }
    if (!present(u"no-bat")) {
        options |= PSIMerger::MERGE_BAT;
    }
    if (!present(u"no-eit")) {
        options |= PSIMerger::MERGE_EIT;
    }
    if (present(u"time-from-merge")) {
        options |= PSIMerger::KEEP_MERGE_TDT;
    }
    else {
        options |= PSIMerger::KEEP_MAIN_TDT;
    }
    _psi_merger.reset(options);

    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PSIMergePlugin::start()
{
    _psi_merger.reset();
    return true;
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PSIMergePlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    if ((_main_label > TSPacketLabelSet::MAX && !pkt_data.hasAnyLabel()) || pkt_data.hasLabel(_main_label)) {
        // This is a packet from the main stream.
        return _psi_merger.feedMainPacket(pkt) ? TSP_OK : TSP_END;
    }
    else if ((_merge_label > TSPacketLabelSet::MAX && !pkt_data.hasAnyLabel()) || pkt_data.hasLabel(_merge_label)) {
        // This is a packet from the merge stream.
        return _psi_merger.feedMergedPacket(pkt) ? TSP_OK : TSP_END;
    }
    else {
        return TSP_OK;
    }
}
