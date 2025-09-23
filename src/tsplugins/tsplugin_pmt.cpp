//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Various transformations on the PMT.
//
//----------------------------------------------------------------------------

#include "tsAbstractTablePlugin.h"
#include "tsPluginRepository.h"
#include "tsServiceDiscovery.h"
#include "tsAudioLanguageOptions.h"
#include "tsPMT.h"
#include "tsCADescriptor.h"
#include "tsStreamIdentifierDescriptor.h"
#include "tsDataBroadcastIdDescriptor.h"
#include "tsRegistrationDescriptor.h"
#include "tsDVBAC3Descriptor.h"
#include "tsDVBEnhancedAC3Descriptor.h"
#include "tsCueIdentifierDescriptor.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PMTPlugin: public AbstractTablePlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PMTPlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual Status processPacket(TSPacket&, TSPacketMetadata&) override;

    private:
        // Description of a new component to add
        struct NewPID {
            PID     pid;
            uint8_t type;

            // Constructor.
            NewPID(PID pid_ = PID_NULL, uint8_t stype_ = 0) :
                pid(pid_),
                type(stype_)
            {
            }
        };

        // Map of new descriptors to add per component.
        using DescriptorListPtr = std::shared_ptr<DescriptorList>;
        using DescriptorListByPID = std::map<PID, DescriptorListPtr>;

        // PMTPlugin instance fields
        ServiceDiscovery     _service {duck, nullptr};   // Service of PMT to modify
        std::vector<PID>     _removed_pids {};           // Set of PIDs to remove from PMT
        std::vector<DID>     _removed_desc_tags {};      // Set of descriptor tags to remove
        std::vector<uint8_t> _removed_stream_types {};   // Set of stream types to remove
        std::vector<REGID>   _removed_registrations {};  // Set of registrations for which PID's are removed
        std::list<NewPID>    _added_pids {};             // List of PID to add
        std::map<PID,PID>    _moved_pids {};             // List of renamed PID's in PMT (key=old, value=new)
        bool                 _set_servid = false;        // Set a new service id
        uint16_t             _new_servid = 0;            // New service id
        bool                 _set_pcrpid = false;        // Set a new PCR PID
        PID                  _new_pcrpid = PID_NULL;     // New PCR PID
        PDS                  _pds = 0;                   // Private data specifier for removed descriptors
        bool                 _add_stream_id = false;     // Add stream_identifier_descriptor on all components
        bool                 _ac3_atsc2dvb = false;      // Modify AC-3 signaling from ATSC to DVB method
        bool                 _eac3_atsc2dvb = false;     // Modify Enhanced-AC-3 signaling from ATSC to DVB method
        bool                 _cleanup_priv_desc = false; // Remove private desc without preceding PDS desc
        DescriptorList       _add_descs {nullptr};       // List of descriptors to add at program level
        DescriptorListByPID  _add_pid_descs {};          // Lists of descriptors to add by PID
        AudioLanguageOptionsVector _languages {};        // Audio languages to set
        std::vector<PID>     _sort_pids {};              // Sorting order of PIDs in PMT
        UStringVector        _sort_languages {};         // Sorting order of audio and subtitles PIDs in PMT

        // Implementation of AbstractTablePlugin.
        virtual void createNewTable(BinaryTable& table) override;
        virtual void modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all) override;

        // Add a descriptor for a given PID in _add_pid_descs.
        void addComponentDescriptor(PID pid, const AbstractDescriptor& desc);

        // Decode an option "pid/value[/hexa]". Hexa is allowed only if hexa is non zero.
        template<typename INT>
        bool decodeOptionForPID(const UChar* parameter_name, size_t parameter_index, PID& pid, INT& value, ByteBlock* hexa = nullptr, INT value_max = std::numeric_limits<INT>::max());

        // Decode options like --set-stream-identifier which add a simple descriptor in a component.
        template<typename DESCRIPTOR, typename INT>
        bool decodeComponentDescOption(const UChar* parameter_name);
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"pmt", ts::PMTPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PMTPlugin::PMTPlugin(TSP* tsp_) :
    AbstractTablePlugin(tsp_, u"Perform various transformations on the PMT", u"[options]", u"PMT")
{
    // We need to define character sets to specify service names.
    duck.defineArgsForCharset(*this);

    option(u"ac3-atsc2dvb");
    help(u"ac3-atsc2dvb",
         u"Change the description of AC-3 audio streams from ATSC to DVB method. "
         u"In details, this means that all components with stream_type 0x81 are "
         u"modified with stream_type 0x06 (PES private data) and an AC-3_descriptor "
         u"is added on this component (if none was already there).");

    option(u"add-ca-descriptor", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"add-ca-descriptor", u"casid/pid[/private-data]",
         u"Add a CA_descriptor at program level in the PMT with the specified CA "
         u"System Id and ECM PID. The optional private data must be a suite of "
         u"hexadecimal digits. Several --add-ca-descriptor options may be specified "
         u"to add several descriptors.");

    option(u"add-pid", 'a', STRING, 0, UNLIMITED_COUNT);
    help(u"add-pid", u"pid/stream_type",
         u"Add the specified PID / stream-type component in the PMT. "
         u"Several --add-pid options may be specified to add several components.");

    option(u"add-registration", 0, UINT32, 0, UNLIMITED_COUNT);
    help(u"add-registration", u"id",
         u"Add a registration_descriptor in the program-level descriptor list in the PMT. "
         u"The value is the format_identifier in registration_descriptor, e.g. 0x43554549 for CUEI.");

    option(u"add-pid-registration", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"add-pid-registration", u"pid/id",
         u"Add a registration_descriptor in the descriptor list of the specified PID in the PMT. "
         u"The value is the format_identifier in registration_descriptor, e.g. 0x43554549 for CUEI.");

    option(u"add-programinfo-id", 0, UINT32, 0, UNLIMITED_COUNT);
    help(u"add-programinfo-id", u"id",
         u"A legacy synonym for --add-registration.");

    option(u"add-stream-identifier");
    help(u"add-stream-identifier",
         u"Add a stream_identifier_descriptor on all components. The component_tag "
         u"are uniquely allocated inside the service. Existing stream_identifier "
         u"descriptors are left unmodified.");

    option(u"audio-language", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"audio-language", AudioLanguageOptions::GetSyntaxString(),
         u"Specifies the language for an audio stream in the PMT. Several options "
         u"can be specified to set the languages of several audio streams.\n\n" +
         AudioLanguageOptions::GetHelpString());

    option(u"cleanup-private-descriptors", 0);
    help(u"cleanup-private-descriptors",
         u"Remove all private descriptors without preceding private_data_specifier descriptor.");

    option(u"eac3-atsc2dvb");
    help(u"eac3-atsc2dvb",
         u"Change the description of Enhanced-AC-3 (aka AC-3+ or DD+) audio streams "
         u"from ATSC to DVB method. In details, this means that all components with "
         u"stream_type 0x87 are modified with stream_type 0x06 (PES private data) "
         u"and an enhanced_AC-3_descriptor is added on this component (if none was "
         u"already there).");

    option(u"new-service-id", 'i', UINT16);
    help(u"new-service-id",
         u"Change the service id in the PMT.");

    option(u"move-pid", 'm', STRING, 0, UNLIMITED_COUNT);
    help(u"move-pid", u"old-pid/new-pid",
         u"Change the PID value of a component in the PMT. Several --move-pid "
         u"options may be specified to move several components.");

    option(u"pds", 0, UINT32);
    help(u"pds",
         u"With option --remove-descriptor, specify the private data specifier "
         u"which applies to the descriptor tag values above 0x80.");

    option(u"pmt-pid", 'p', PIDVAL);
    help(u"pmt-pid",
         u"Specify the PID carrying the PMT to modify. All PMT's in this PID will be "
         u"modified. Options --pmt-pid and --service are mutually exclusive. If "
         u"neither are specified, the first service in the PAT is used.");

    option(u"pcr-pid", 0, PIDVAL);
    help(u"pcr-pid",
         u"Change the PCR PID value in the PMT.");

    option(u"remove-descriptor", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"remove-descriptor",
         u"Remove from the PMT all descriptors with the specified tag. Several "
         u"--remove-descriptor options may be specified to remove several types of "
         u"descriptors. See also option --pds.");

    option(u"remove-pid", 'r', PIDVAL, 0, UNLIMITED_COUNT);
    help(u"remove-pid", u"pid1[-pid2]",
         u"Remove the component with the specified PID's from the PMT. Several "
         u"--remove-pid options may be specified to remove several components.");

    option(u"remove-stream-type", 0, UINT8, 0, UNLIMITED_COUNT);
    help(u"remove-stream-type", u"value1[-value2]",
         u"Remove all components with a stream type matching the specified value (or in the specified range of values). "
         u"Several --remove-stream-type options may be specified.");

    option(u"remove-with-registration", 0, UINT32, 0, UNLIMITED_COUNT);
    help(u"remove-with-registration", u"value1[-value2]",
         u"Remove all components with a registration descriptor containing the specified value (or in the specified range of values). "
         u"Several --remove-with-registration options may be specified.");

    option(u"service", 's', STRING);
    help(u"service", u"name-or-id",
         u"Specify the service the PMT of which must be modified. If the argument is "
         u"an integer value (either decimal or hexadecimal), it is interpreted as a "
         u"service id. Otherwise, it is interpreted as a service name, as specified "
         u"in the SDT. The name is not case sensitive and blanks are ignored. "
         u"Options --pmt-pid and --service are mutually exclusive. If neither are "
         u"specified, the first service in the PAT is used.");

    option(u"set-cue-type", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"set-cue-type", u"pid/type",
         u"In the component with the specified PID, add an SCTE 35 cue_identifier "
         u"descriptor with the specified cue stream type. Several --set-cue-type "
         u"options may be specified.");

    option(u"set-data-broadcast-id", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"set-data-broadcast-id", u"pid/id[/selector]",
         u"In the component with the specified PID, add a data_broadcast_id_descriptor "
         u"with the specified data_broadcast_id. The optional selector is a suite of "
         u"hexadecimal characters representing the content of the selector bytes. "
         u"Several --set-data-broadcast-id options may be specified.");

    option(u"set-stream-identifier", 0, STRING, 0, UNLIMITED_COUNT);
    help(u"set-stream-identifier", u"pid/id",
         u"In the component with the specified PID, add a stream_identifier_descriptor "
         u"with the specified id. Several --set-stream-identifier options may be "
         u"specified.");

    option(u"sort-languages", 0, STRING);
    help(u"sort-languages", u"lang1,lang2,...",
         u"Sort the elementary streams carrying audio and subtitles in the specified order of languages. "
         u"The languages must be 3-letter ISO-639 codes.");

    option(u"sort-pids", 0, STRING);
    help(u"sort-pids", u"pid1,pid2,...",
         u"Sort the elementary streams in the specified order of PID's. "
         u"Non-existent PID's are ignored. "
         u"Unlisted PID's, if any, are placed after the others.");
}


//----------------------------------------------------------------------------
// Add a descriptor for a given PID in _add_pid_descs.
//----------------------------------------------------------------------------

void ts::PMTPlugin::addComponentDescriptor(PID pid, const AbstractDescriptor& desc)
{
    // Get or create descriptor list for the component.
    if (_add_pid_descs[pid] == nullptr) {
        _add_pid_descs[pid] = std::make_shared<DescriptorList>(nullptr);
    }

    // Add the new descriptor.
    _add_pid_descs[pid]->add(duck, desc);
}


//----------------------------------------------------------------------------
// Decode an option "pid/param[/hexa]".
//----------------------------------------------------------------------------

template<typename INT>
bool ts::PMTPlugin::decodeOptionForPID(const UChar* parameter_name, size_t parameter_index, PID& pid, INT& param, ByteBlock* hexa, INT value_max)
{
    // Get the parameter string value.
    const UString str(value(parameter_name, u"", parameter_index));

    // Get slash-separated fields.
    UStringVector fields;
    str.split(fields, u'/');

    // Check number of fields.
    const size_t count = fields.size();
    bool ok = (hexa == nullptr && count == 2) || (hexa != nullptr && (count == 2 || count == 3));

    // Get first two parameters.
    if (ok) {
        uint64_t v1 = 0, v2 = 0;
        ok = fields[0].toInteger(v1, u",") &&
             fields[1].toInteger(v2, u",") &&
             v1 < uint64_t(PID_MAX) &&
             v2 <= uint64_t(value_max);
        if (ok) {
            pid = PID(v1);
            param = INT(v2);
        }
    }

    // Get third parameter.
    if (ok && hexa != nullptr) {
        if (count < 3) {
            hexa->clear();
        }
        else {
            ok = fields[2].hexaDecode(*hexa);
        }
    }

    // Process errors.
    if (!ok) {
        error(u"invalid value \"%s\" for --%s", str, parameter_name);
    }
    return ok;
}


//----------------------------------------------------------------------------
// Decode options like --set-stream-identifier which add a simple descriptor in a component.
//----------------------------------------------------------------------------

template<typename DESCRIPTOR, typename INT>
bool ts::PMTPlugin::decodeComponentDescOption(const UChar* parameter_name)
{
    // Loop on all option values.
    const size_t opt_count = count(parameter_name);
    for (size_t n = 0; n < opt_count; n++) {
        PID pid = PID_NULL;
        INT param = 0;
        if (decodeOptionForPID(parameter_name, n, pid, param)) {
            // Add a new descriptor of the requested type.
            addComponentDescriptor(pid, DESCRIPTOR(param));
        }
        else {
            return false;
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PMTPlugin::start()
{
    _service.clear();
    _added_pids.clear();
    _moved_pids.clear();
    _add_descs.clear();
    _add_pid_descs.clear();
    _sort_pids.clear();
    _sort_languages.clear();

    // Get option values
    duck.loadArgs(*this);
    _set_servid = present(u"new-service-id");
    getIntValue(_new_servid, u"new-service-id");
    _set_pcrpid = present(u"pcr-pid");
    getIntValue(_new_pcrpid, u"pcr-pid");
    getIntValue(_pds, u"pds");
    _ac3_atsc2dvb = present(u"ac3-atsc2dvb");
    _eac3_atsc2dvb = present(u"eac3-atsc2dvb");
    _add_stream_id = present(u"add-stream-identifier");
    _cleanup_priv_desc = present(u"cleanup-private-descriptors");
    getIntValues(_removed_pids, u"remove-pid");
    getIntValues(_removed_desc_tags, u"remove-descriptor");
    getIntValues(_removed_stream_types, u"remove-stream-type");
    getIntValues(_removed_registrations, u"remove-with-registration");

    // Get list of components to add
    size_t opt_count = count(u"add-pid");
    for (size_t n = 0; n < opt_count; n++) {
        PID pid = PID_NULL;
        uint8_t stype = 0;
        if (decodeOptionForPID(u"add-pid", n, pid, stype)) {
            _added_pids.push_back(NewPID(pid, stype));
        }
        else {
            return false;
        }
    }

    // Get suboptions for component to add, type of identifier and tag
    if (!decodeComponentDescOption<StreamIdentifierDescriptor, uint8_t>(u"set-stream-identifier") ||
        !decodeComponentDescOption<CueIdentifierDescriptor, uint8_t>(u"set-cue-type"))
    {
        return false;
    }

    // Get list of data_broadcast_id_descriptors to add
    opt_count = count(u"set-data-broadcast-id");
    for (size_t n = 0; n < opt_count; n++) {
        PID pid = PID_NULL;
        DataBroadcastIdDescriptor desc;
        if (decodeOptionForPID(u"set-data-broadcast-id", n, pid, desc.data_broadcast_id, &desc.private_data)) {
            addComponentDescriptor(pid, desc);
        }
        else {
            return false;
        }
    }

    // Get list of components to move
    opt_count = count(u"move-pid");
    for (size_t n = 0; n < opt_count; n++) {
        PID opid = PID_NULL;
        PID npid = PID_NULL;
        if (decodeOptionForPID(u"move-pid", n, opid, npid, nullptr, PID(PID_MAX - 1))) {
            _moved_pids[PID(opid)] = PID(npid);
        }
        else {
            return false;
        }
    }

    // Get audio languages to set.
    if (!_languages.getFromArgs(*this, u"audio-language")) {
        return false;
    }

    // Get list of descriptors to add
    UStringVector cadescs;
    getValues(cadescs, u"add-ca-descriptor");
    if (!CADescriptor::AddFromCommandLine(duck, _add_descs, cadescs)) {
        return false;
    }
    opt_count = count(u"add-programinfo-id");
    for (size_t n = 0; n < opt_count; n++) {
        _add_descs.add(duck, RegistrationDescriptor(intValue<uint32_t>(u"add-programinfo-id", 0, n)));
    }
    opt_count = count(u"add-registration");
    for (size_t n = 0; n < opt_count; n++) {
        _add_descs.add(duck, RegistrationDescriptor(intValue<uint32_t>(u"add-registration", 0, n)));
    }
    opt_count = count(u"add-pid-registration");
    for (size_t n = 0; n < opt_count; n++) {
        PID pid = PID_NULL;
        uint32_t id = 0;
        if (decodeOptionForPID(u"add-pid-registration", n, pid, id)) {
            addComponentDescriptor(pid, RegistrationDescriptor(id));
        }
        else {
            return false;
        }
    }

    // Get elementary streams sorting criteria.
    if (!value(u"sort-pids").toIntegers(_sort_pids, u"", u",", 0, u".", 0, PID_MAX - 1)) {
        error(u"invalid list of PID's in --sort-pids");
        return false;
    }
    value(u"sort-languages").toLower().split(_sort_languages, COMMA, true, true);
    for (const auto& lang : _sort_languages) {
        if (lang.length() != 3) {
            error(u"invalid language '%s' in --sort-languages", lang);
            return false;
        }
    }
    if (!_sort_pids.empty() && !_sort_languages.empty()) {
        error(u"--sort-pids and --sort-languages are mutually exclusive");
        return false;
    }

    // Get PMT PID or service description
    if (present(u"pmt-pid") && present(u"service")) {
        error(u"options --pmt-pid and --service are mutually exclusive");
        return false;
    }
    if (present(u"pmt-pid")) {
        // A PMT PID is specified, we are now ready to modify all PMT's in this PID
        _service.setPMTPID(intValue<PID>(u"pmt-pid"));
    }
    else {
        _service.set(value(u"service"));
    }

    // Start superclass.
    return AbstractTablePlugin::start();
}


//----------------------------------------------------------------------------
// Invoked by the superclass to create an empty table.
//----------------------------------------------------------------------------

void ts::PMTPlugin::createNewTable(BinaryTable& table)
{
    PMT pmt;

    // If we know the expected service id, this is the one we need to create.
    if (_service.hasId()) {
        pmt.service_id = _service.getId();
    }

    pmt.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Invoked by the superclass when a table is found in the target PID.
//----------------------------------------------------------------------------

void ts::PMTPlugin::modifyTable(BinaryTable& table, bool& is_target, bool& reinsert, bool& replace_all)
{
    // If not the PMT we are looking for, reinsert without modification.
    is_target = table.tableId() == TID_PMT && (!_service.hasId() || table.tableIdExtension() == _service.getId());
    if (!is_target) {
        return;
    }

    // Process the PMT.
    PMT pmt(duck, table);
    if (!pmt.isValid()) {
        warning(u"found invalid PMT");
        reinsert = false;
        return;
    }

    // ---- Global non-descriptor data

    // Modify service id
    if (_set_servid) {
        pmt.service_id = _new_servid;
    }

    // Modify PCR PID
    if (_set_pcrpid) {
        pmt.pcr_pid = _new_pcrpid;
    }

    // ---- Do removal first (otherwise it could remove things we add...)

    // Remove components by PID.
    for (auto rpid : _removed_pids) {
        pmt.streams.erase(rpid);
    }

    // Remove components by stream type.
    for (auto rtype : _removed_stream_types) {
        for (auto str = pmt.streams.begin(); str != pmt.streams.end(); ) {
            if (str->second.stream_type == rtype) {
                str = pmt.streams.erase(str);
            }
            else {
                ++str;
            }
        }
    }

    // Remove components containing specific registration descriptors.
    for (auto regid : _removed_registrations) {
        for (auto str = pmt.streams.begin(); str != pmt.streams.end(); ) {
            if (str->second.descs.containsRegistration(regid)) {
                str = pmt.streams.erase(str);
            }
            else {
                ++str;
            }
        }
    }

    // Remove descriptors
    for (auto tag : _removed_desc_tags) {
        pmt.descs.removeByTag(tag, _pds);
        for (auto& smi : pmt.streams) {
            smi.second.descs.removeByTag(tag, _pds);
        }
    }

    // Remove private descriptors without preceding PDS descriptor
    if (_cleanup_priv_desc) {
        pmt.descs.removeInvalidPrivateDescriptors();
        for (auto& smi : pmt.streams) {
            smi.second.descs.removeInvalidPrivateDescriptors();
        }
    }

    // ---- Add components and descriptors

    // Add new components
    for (const auto& it : _added_pids) {
        pmt.streams[it.pid].stream_type = it.type;
    }

    // Add new descriptors at program level.
    pmt.descs.add(_add_descs);

    // Add descriptors on components.
    for (const auto& it : _add_pid_descs) {
        const PID pid = it.first;
        const DescriptorList& dlist(*it.second);

        auto comp_it = pmt.streams.find(pid);
        if (comp_it == pmt.streams.end()) {
            warning(u"PID %n not found in PMT", pid);
        }
        else {
            comp_it->second.descs.add(dlist);
        }
    }

    // Modify audio languages
    _languages.apply(duck, pmt);

    // Modify AC-3 signaling from ATSC to DVB method
    if (_ac3_atsc2dvb) {
        for (auto& smi : pmt.streams) {
            if (smi.second.stream_type == ST_AC3_AUDIO) {
                smi.second.stream_type = ST_PES_PRIV;
                if (smi.second.descs.search(DID_DVB_AC3) == smi.second.descs.count()) {
                    // No AC-3_descriptor present in this component, add one.
                    smi.second.descs.add(duck, DVBAC3Descriptor());
                }
            }
        }
    }

    // Modify Enhanced-AC-3 signaling from ATSC to DVB method
    if (_eac3_atsc2dvb) {
        for (auto& smi : pmt.streams) {
            if (smi.second.stream_type == ST_EAC3_AUDIO) {
                smi.second.stream_type = ST_PES_PRIV;
                if (smi.second.descs.search (DID_DVB_ENHANCED_AC3) == smi.second.descs.count()) {
                    // No enhanced_AC-3_descriptor present in this component, add one.
                    smi.second.descs.add(duck, DVBEnhancedAC3Descriptor());
                }
            }
        }
    }

    // Add stream_identifier_descriptor on all components.
    // Do this late to avoid clashing with descriptors we added.
    if (_add_stream_id) {

        // First, look for existing descriptors, collect component tags.
        std::bitset<256> ctags;
        for (auto& smi : pmt.streams) {
            const DescriptorList& dlist(smi.second.descs);
            for (size_t i = dlist.search(DID_DVB_STREAM_ID); i < dlist.count(); i = dlist.search(DID_DVB_STREAM_ID, i + 1)) {
                const StreamIdentifierDescriptor sid(duck, dlist[i]);
                if (sid.isValid()) {
                    ctags.set(sid.component_tag);
                }
            }
        }

        // Then, add a stream_identifier_descriptor on all components which do not have one.
        for (auto& smi : pmt.streams) {
            DescriptorList& dlist(smi.second.descs);
            // Skip components already containing a stream_identifier_descriptor
            if (dlist.search(DID_DVB_STREAM_ID) < dlist.count()) {
                continue;
            }
            // Allocate a new component tag
            StreamIdentifierDescriptor sid;
            for (size_t i = 0; i < ctags.size(); i++) {
                if (!ctags.test(i)) {
                    sid.component_tag = uint8_t(i);
                    ctags.set(i);
                    break;
                }
            }
            // Add the stream_identifier_descriptor in the component
            dlist.add(duck, sid);
        }
    }

    // ---- PID remapping

    for (const auto& mpid : _moved_pids) {
        // Check if component exists
        if (mpid.first != mpid.second && pmt.streams.contains(mpid.first)) {
            pmt.streams[mpid.second] = pmt.streams[mpid.first];
            pmt.streams.erase(mpid.first);
        }
    }

    // --- Sorting elementary streams

    if (!_sort_pids.empty()) {
        pmt.streams.setOrder(_sort_pids);
    }
    if (!_sort_languages.empty()) {
        std::vector<PID> input;  // input sort order
        std::vector<PID> output; // output sort order, video PID's come first
        std::vector<PID> other;  // other PID's
        // Classify input PID's. Video PID's are placed first in output PMT.
        pmt.streams.getOrder(input);
        for (PID pid : input) {
            if (pmt.streams[pid].isVideo(duck)) {
                output.push_back(pid);
            }
            else {
                other.push_back(pid);
            }
        }
        // Sort other PID's by language.
        for (const auto& lang : _sort_languages) {
            for (PID& pid : other) {
                const auto& stream(pmt.streams[pid]);
                if (stream.descs.searchLanguage(duck, lang) < stream.descs.size()) {
                    // This audio or subtitles PID has this language.
                    output.push_back(pid);
                    pid = PID_NULL;
                }
            }
        }
        // Append PID's with unspecified language.
        for (PID pid : other) {
            if (pid != PID_NULL) {
                output.push_back(pid);
            }
        }
        // And sort the PID's...
        pmt.streams.setOrder(output);
    }

    // Reserialize modified PMT.
    pmt.serialize(duck, table);
}


//----------------------------------------------------------------------------
// Packet processing method
//----------------------------------------------------------------------------

ts::ProcessorPlugin::Status ts::PMTPlugin::processPacket(TSPacket& pkt, TSPacketMetadata& pkt_data)
{
    // As long as the PMT PID is unknown, pass packets to the service discovery.
    if (!_service.hasPMTPID()) {
        _service.feedPacket(pkt);
    }

    // Abort when a service was specified and we realize it does not exist.
    if (_service.nonExistentService()) {
        return TSP_END;
    }

    // While we don't know which PID to modify, drop all packets to avoid transmitting partial unmodified table.
    if (!_service.hasPMTPID()) {
        return TSP_DROP;
    }

    // The first time we get the PMT PID, set it in the superclass.
    // In fact, set it all the time but this won't do anything when the PID is already known.
    setPID(_service.getPMTPID());

    // Finally, let the superclass do the job.
    return AbstractTablePlugin::processPacket(pkt, pkt_data);
}
