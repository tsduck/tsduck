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
//  Remove or merge sections from various PID's.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTablePatchXML.h"
#include "tsSectionDemux.h"
#include "tsPacketizer.h"
#include "tsAlgorithm.h"
#include "tsBoolPredicate.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class SectionsPlugin:
        public ProcessorPlugin,
        private SectionHandlerInterface,
        private SectionProviderInterface
    {
        TS_NOBUILD_NOCOPY(SectionsPlugin);
    public:
        // Implementation of plugin API
        SectionsPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Command line options.
        bool                   _section_stuffing;
        bool                   _use_null_pid;
        bool                   _reverse_eitd;
        bool                   _keep_selected;
        bool                   _selections_present;   // there are selection options in the command line
        MultiBoolPredicate     _predicate;            // global "and" / "or" on all criteria, see option --and
        MonoBoolPredicate      _valid_predicate;      // see method condition()
        BoolPredicate          _cond_predicate;       // see method condition()
        size_t                 _max_buffered_sections;
        PIDSet                 _input_pids;
        PID                    _output_pid;
        std::set<TID>          _tids;
        std::set<uint16_t>     _exts;
        std::set<uint32_t>     _etids;
        std::set<uint8_t>      _versions;
        std::set<uint8_t>      _section_numbers;
        std::vector<ByteBlock> _contents;
        std::vector<ByteBlock> _contents_masks;

        // Working data.
        std::list<SectionPtr> _sections;
        SectionDemux          _demux;
        Packetizer            _packetizer;
        TablePatchXML         _patch_xml;

        // Compute a condition in the chain of _predicate.
        // - valid: the condition needs to be checked (eg. there are some tids to remove).
        // - cond: the condition itself (eg. this section has a tid to remove).
        bool condition(bool valid, bool cond) const { return _cond_predicate(_valid_predicate(valid), cond); }

        // Check if a section matches any selected leading content.
        bool matchContent(const Section& section) const;

        // Implementation of SectionHandlerInterface.
        virtual void handleSection(SectionDemux& demux, const Section& section) override;

        // Implementation of SectionProviderInterface.
        virtual void provideSection(SectionCounter counter, SectionPtr& section) override;
        virtual bool doStuffing() override;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"sections", ts::SectionsPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::SectionsPlugin::SectionsPlugin(TSP* tsp_) :
    ProcessorPlugin(tsp_, u"Remove, keep or merge sections from various PID's", u"[options]"),
    _section_stuffing(false),
    _use_null_pid(false),
    _reverse_eitd(false),
    _keep_selected(false),
    _selections_present(false),
    _predicate(nullptr),
    _valid_predicate(nullptr),
    _cond_predicate(nullptr),
    _max_buffered_sections(1024), // hard-coded for now
    _input_pids(),
    _output_pid(PID_NULL),
    _tids(),
    _exts(),
    _etids(),
    _versions(),
    _section_numbers(),
    _contents(),
    _contents_masks(),
    _sections(),
    _demux(duck, nullptr, this),
    _packetizer(duck, PID_NULL, this),
    _patch_xml(duck)
{
    option(u"and", 'a');
    help(u"and",
         u"Remove/keep a section when all conditions are true. "
         u"By default, a section is removed/kept as soon as one condition is true.");

    option(u"etid", 0, UINT32, 0, UNLIMITED_COUNT, 0, 0x00FFFFFF);
    help(u"etid", u"id1[-id2]",
         u"Remove/keep all sections with the corresponding \"extended table id\" values. "
         u"The value is a combination of the table id and the table id extension. "
         u"For example, the option -e 0x4A1234 removes/keeps all BAT sections (table id 0x4A) "
         u"for bouquet id 0x1234 (table id extension). "
         u"Several options --etid can be specified. "
         u"See also option --reverse-etid.");

    option(u"keep", 'k');
    help(u"keep",
         u"Keep selected sections and remove others. "
         u"The selection options are --tid, --etid, --version, etc. "
         u"By default, when selection options are present, the selected sections are removed. "
         u"If no selection option is present, the sections are simply merged from the various input PID's.");

    option(u"null-pid-reuse", 'n');
    help(u"null-pid-reuse",
         u"With this option, null packets can be replaced by packets for the "
         u"output PID. By default, only packets from input PID's are replaced "
         u"by output packets. This option may need to be used when --stuffing "
         u"is specified and the input PID's contained packed sections. In that "
         u"case, the output payload can be larger than the input and additional "
         u"packets must be used.");

    option(u"output-pid", 'o', PIDVAL);
    help(u"output-pid",
         u"Specifies the output PID. By default, the first input PID on the "
         u"command line is used. If the output PID is different from all input "
         u"PID's and this output PID already exists in the transport stream, "
         u"an error is generated.");

    option(u"pid", 'p', PIDVAL, 1, UNLIMITED_COUNT);
    help(u"pid", u"pid1[-pid2]",
         u"Specify input PID's. More than one input PID can be specified. "
         u"All sections from all input PID's are merged into the output PID. "
         u"At least one input PID must be specified. ");

    option(u"reverse-etid", 'r');
    help(u"reverse-etid",
         u"With option --etid, reverse the table id and the table id extension parts in the \"extended table id\" values. "
         u"With this option, the values in --etid are 0xEEEETT instead of 0xTTEEEE where "
         u"'TT' is the table id part and 'EEEE' the table id extension part. "
         u"This option can be useful when specifying ranges of values. "
         u"For instance, the option '--etid 0x4A1234-0x4A1250' removes/keeps BAT sections "
         u"(table id 0x4A) for all service ids in the range 0x1234 to 0x1250. "
         u"On the other hand, the options '--etid 0x12344E-0x12346F --reverse-etid' remove/keep all EIT "
         u"sections (table ids 0x4E to 0x6F) for the service id 0x1234.");

    option(u"section-content", 0, HEXADATA, 0, UNLIMITED_COUNT, 1);
    help(u"section-content",
         u"Remove/keep all sections the binary content of which starts with the specified binary data. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"See also option --section-mask to specify selected bits or bytes only. "
         u"Several options --section-content can be specified.");

    option(u"section-mask", 0, HEXADATA, 0, UNLIMITED_COUNT, 1);
    help(u"section-mask",
         u"With --section-content, specify a mask of meaningful bits in the binary data that must match the beginning of the section. "
         u"The value must be a string of hexadecimal digits specifying any number of bytes. "
         u"If omitted or shorter than the --section-content parameter, the mask is implicitely padded with FF bytes. "
         u"If several options --section-content are specified, several options --section-mask can be specified. "
         u"The first mask applies to the first content, the second mask to the second content, etc. "
         u"If there are less masks than contents, the last mask is implicitly repeated.");

    option(u"section-number", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"section-number", u"num1[-num2]",
         u"Remove/keep all sections with the corresponding section number. "
         u"Several options --section-number can be specified.");

    option(u"stuffing", 's');
    help(u"stuffing",
         u"Insert stuffing at end of each section, up to the next TS packet "
         u"boundary. By default, sections are packed and start in the middle "
         u"of a TS packet, after the previous section. Note, however, that "
         u"section headers are never scattered over a packet boundary.");

    option(u"tid", 't', UINT8, 0, UNLIMITED_COUNT);
    help(u"tid", u"id1[-id2]",
         u"Remove/keep all sections with the corresponding table id. "
         u"Several options --tid can be specified.");

    option(u"tid-ext", 'e', UINT16, 0, UNLIMITED_COUNT);
    help(u"tid-ext", u"id1[-id2]",
         u"Remove/keep all sections with the corresponding table id extension. "
         u"Several options --tid-ext can be specified.");

    option(u"version", 'v', INTEGER, 0, UNLIMITED_COUNT, 0, 31);
    help(u"version", u"v1[-v2]",
         u"Remove/keep all sections with the corresponding versions. "
         u"Several options --version can be specified.");

    // Slightly amend the semantics of --patch-xml here.
    _patch_xml.defineArgs(*this);
    help(u"patch-xml",
         u"Specify an XML patch file which is applied to all sections on the fly. "
         u"Here, the behavior of --patch-xml is slightly different, compared to other commands or plugins. "
         u"While XML representation and patch normally apply to a complete table, they process one single section here. "
         u"This means that the result of the patch must fit into one single section. "
         u"Otherwise, only the first section of the result is kept (with the original section number of the input section). "
         u"If the name starts with \"<?xml\", it is considered as \"inline XML content\". "
         u"Several --patch-xml options can be specified. "
         u"Patch files are sequentially applied on each section.");
}


//----------------------------------------------------------------------------
// Get command line options.
//----------------------------------------------------------------------------

bool ts::SectionsPlugin::getOptions()
{
    _section_stuffing = present(u"stuffing");
    _use_null_pid = present(u"null-pid-reuse");
    _reverse_eitd = present(u"reverse-etid");
    _keep_selected = present(u"keep");
    _output_pid = intValue(u"output-pid", intValue<PID>(u"pid", PID_NULL, 0));
    getIntValues(_input_pids, u"pid");
    getIntValues(_tids, u"tid");
    getIntValues(_exts, u"tid-ext");
    getIntValues(_etids, u"etid");
    getIntValues(_versions, u"version");
    getIntValues(_section_numbers, u"section-number");

    _contents.resize(count(u"section-content"));
    for (size_t i = 0; i < _contents.size(); ++i) {
        getHexaValue(_contents[i], u"section-content", ByteBlock(), i);
    }

    _contents_masks.resize(count(u"section-mask"));
    for (size_t i = 0; i < _contents_masks.size(); ++i) {
        getHexaValue(_contents_masks[i], u"section-mask", ByteBlock(), i);
    }

    if (_contents_masks.size() > _contents.size()) {
        warning(u"more --section-mask than --section-content, extraneous masks are ignored");
        _contents_masks.resize(_contents.size());
    }
    else if (_contents_masks.size() < _contents.size()) {
        // Use the last mask for missing ones. If no mask specified, use a default one.
        const ByteBlock def(_contents_masks.empty() ? ByteBlock(1, 0xFF) : _contents_masks.back());
        _contents_masks.resize(_contents.size(), def);
    }

    // If there any section to remove/keep?
    _selections_present = !_tids.empty() || !_exts.empty() || !_etids.empty() || !_versions.empty() || !_section_numbers.empty() || !_contents.empty();

    if (present(u"and")) {
        // Global "AND" on all (!valid || condition)
        _predicate = MultiAnd;
        _valid_predicate = Not;
        _cond_predicate = Or;
    }
    else {
        // Global "OR" on all (valid && condition)
        _predicate = MultiOr;
        _valid_predicate = Identity;
        _cond_predicate = And;
    }

    return _patch_xml.loadArgs(duck, *this);
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::SectionsPlugin::start()
{
    _demux.reset();
    _demux.setPIDFilter(_input_pids);
    _packetizer.reset();
    _packetizer.setPID(_output_pid);
    _sections.clear();
    return _patch_xml.loadPatchFiles();
}


//----------------------------------------------------------------------------
// Check if a section matches any selected leading content.
//----------------------------------------------------------------------------

bool ts::SectionsPlugin::matchContent(const Section& section) const
{
    assert(_contents.size() == _contents_masks.size());
    for (size_t i = 0; i < _contents.size(); ++i) {
        if (section.matchContent(_contents[i], _contents_masks[i])) {
            return true;
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Shall we perform section stuffing right now?
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

bool ts::SectionsPlugin::doStuffing()
{
    return _section_stuffing;
}


//----------------------------------------------------------------------------
// Invoked when the packetizer needs a new section to insert.
// Implementation of SectionProviderInterface.
//----------------------------------------------------------------------------

void ts::SectionsPlugin::provideSection(SectionCounter counter, SectionPtr& section)
{
    if (_sections.empty()) {
        // No section to provide.
        section.clear();
    }
    else {
        // Remove one section from the queue for insertion.
        section = _sections.front();
        _sections.pop_front();
    }
}


//----------------------------------------------------------------------------
// Invoked by the demux when a complete section is available.
// Implementation of SectionHandlerInterface.
//----------------------------------------------------------------------------

void ts::SectionsPlugin::handleSection(SectionDemux& demux, const Section& section)
{
    // Section characteristics.
    const TID tid = section.tableId();
    const bool is_long = section.isLongSection();
    const uint16_t ext = section.tableIdExtension();
    const uint32_t etid = _reverse_eitd ? ((uint32_t(ext) << 8) | tid) : ((uint32_t(tid) << 16) | ext);

    // Detect sections to be selected. This can be an "and" or an "or" on the conditions.
    const bool selected = _selections_present && _predicate({
        condition(!_tids.empty(), Contains(_tids, tid)),
        condition(is_long && !_exts.empty(), Contains(_exts, ext)),
        condition(is_long && !_etids.empty(), Contains(_etids, etid)),
        condition(is_long && !_versions.empty(), Contains(_versions, section.version())),
        condition(is_long && !_section_numbers.empty(), Contains(_section_numbers, section.sectionNumber())),
        condition(!_contents.empty(), matchContent(section)),
    });

    if (!_selections_present || (_keep_selected && selected) || (!_keep_selected && !selected)) {
        // At this point, we need to keep the section.

        // Build a copy of it for insertion in the queue.
        SectionPtr sp(new Section(section, ShareMode::SHARE));
        CheckNonNull(sp.pointer());

        // Process XML patching.
        if (!_patch_xml.applyPatches(sp)) {
            // Patch error, drop that section. Errors are displayed in applyPatches().
            return;
        }

        // Now insert the section in the queue for the packetizer (if not deleted by the patch file).
        if (!sp.isNull()) {
            _sections.push_back(sp);
        }
    }
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::SectionsPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    const PID pid = pkt.getPID();

    // If the output PID is not an input one and already exists, this is an error.
    if (pid == _output_pid && !_input_pids.test(_output_pid)) {
        tsp->error(u"output PID 0x%X (%d) already present in the stream", {_output_pid, _output_pid});
        return TSP_END;
    }

    // Filter sections to process / merge.
    _demux.feedPacket(pkt);

    // Fool-proof check. It the input PID's contain packed sections and
    // we perform section stuffing and we do not reuse null packets or
    // there are not enough null packets, we may accumulate more and
    // more sections until the memory is exhausted.
    if (_sections.size() > _max_buffered_sections) {
        tsp->error(u"too many accumulated buffered sections, not enough space in output PID");
        return TSP_END;
    }

    // Replace packets from all input PID's using packetizer.
    if (_input_pids.test(pid) || (_use_null_pid && pid == PID_NULL)) {
        _packetizer.getNextPacket(pkt);
    }

    return TSP_OK;
}
