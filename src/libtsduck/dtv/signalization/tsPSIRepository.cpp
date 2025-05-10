//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsAlgorithm.h"
#include "tsCerrReport.h"
#include "tsTextTable.h"
#include "tsSysUtils.h"

TS_DEFINE_SINGLETON(ts::PSIRepository);


//----------------------------------------------------------------------------
// Repository singleton constructor.
//----------------------------------------------------------------------------

ts::PSIRepository::PSIRepository()
{
    CERR.debug(u"creating PSIRepository");

    // Load all table names from a ".names" file.
    const NamesPtr tid_repo = Names::GetSection(u"dtv", u"TableId", true);
    const NamesPtr did_repo = Names::GetSection(u"dtv", u"DescriptorId", true);
    tid_repo->visit(this);
    did_repo->visit(this);

    // Subscribe to further modifications (merge of extension files).
    tid_repo->subscribe(this);
    did_repo->subscribe(this);
}


//----------------------------------------------------------------------------
// Constructors to register extension files.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterXML::RegisterXML(const UString& file_name)
{
    CERR.debug(u"registering XML file %s", file_name);
    PSIRepository::Instance()._xml_extension_files.push_back(file_name);
}


//----------------------------------------------------------------------------
// Load all table and descriptor names from DTV names file
// (executed in PSIRepository constructor)
//----------------------------------------------------------------------------

bool ts::PSIRepository::handleNameValue(const Names& section, Names::uint_t value, const UString& name)
{
    if (section.sectionName().similar(u"TableId")) {
        // Register a table name. Decode the extended table id.
        const Standards std = Standards((value >> 16) & 0xFFFF);
        const CASFamily cas = CASFamily((value >> 8) & 0xFF);
        const TID tid = TID(value & 0xFF);
        CASID min_cas = FirstCASId(cas);
        CASID max_cas = LastCASId(cas);

        // Update existing entries.
        bool existed = false;
        const auto bounds(_tables_by_tid.equal_range(tid));
        for (auto it = bounds.first; it != bounds.second; ++it) {
            const auto& tc(it->second);
            if ((std == tc->standards || bool(std & tc->standards)) && min_cas >= tc->min_cas && max_cas <= tc->max_cas) {
                // Found a compatible entry.
                existed = true;
                tc->display_name = name;
            }
        }

        // Create one entry if not found.
        if (!existed) {
            TableClassPtr tc = std::make_shared<TableClass>();
            tc->standards = std;
            tc->min_cas = min_cas;
            tc->max_cas = max_cas;
            tc->display_name = name;
            _tables_by_tid.insert(std::make_pair(tid, tc));
        }
    }
    else if (section.sectionName().similar(u"DescriptorId")) {
        // Register a descriptor name. The value is an EDID.
        const EDID edid(value);

        // Update existing entries.
        bool existed = false;
        const auto bounds(_descriptors_by_xdid.equal_range(edid.xdid()));
        for (auto it = bounds.first; it != bounds.second; ++it) {
            if (it->second->edid == edid) {
                // Found a compatible entry.
                existed = true;
                it->second->display_name = name;
            }
        }

        // Create one entry if not found.
        if (!existed) {
            DescriptorClassPtr dc = std::make_shared<DescriptorClass>();
            dc->edid = edid;
            dc->display_name = name;
            _descriptors_by_xdid.insert(std::make_pair(edid.xdid(), dc));
        }
    }

    // Continue visting the names.
    return true;
}


//----------------------------------------------------------------------------
// Constructors to register a fully or partially implemented table.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterTable::RegisterTable(TableFactory factory,
                                                std::type_index index,
                                                const std::vector<TID>& tids,
                                                Standards standards,
                                                const UString& xml_name,
                                                DisplaySectionFunction display,
                                                LogSectionFunction log,
                                                std::initializer_list<PID> pids,
                                                CASID min_cas,
                                                CASID max_cas)
{
    CERR.log(2, u"registering table <%s>", xml_name);
    PSIRepository& repo(PSIRepository::Instance());
    bool xml_done = false;

    // Separately store each TID. They may not hold the same content in the end (eg. distinct display names for EIT).
    for (auto tid : tids) {
        TableClassPtr tc;

        // Search an existing entry.
        const auto bounds(repo._tables_by_tid.equal_range(tid));
        for (auto it = bounds.first; tc == nullptr && it != bounds.second; ++it) {
            const auto& tc1(it->second);
            if ((standards == tc1->standards || bool(standards & tc1->standards)) && min_cas >= tc1->min_cas && max_cas <= tc1->max_cas) {
                // Found a compatible entry.
                tc = tc1;
            }
        }

        // Build a new entry if none found.
        if (tc == nullptr) {
            tc = std::make_shared<TableClass>();
            repo._tables_by_tid.insert(std::make_pair(tid, tc));
        }

        // Fill the entry with new data.
        tc->index = index;
        tc->standards = standards;
        tc->min_cas = min_cas;
        tc->max_cas = max_cas;
        tc->factory = factory;
        tc->display = display;
        tc->log = log;
        tc->xml_name = xml_name;
        tc->pids.insert(pids);

        // Store the first description as XML name.
        if (!xml_done && !xml_name.empty()) {
            xml_done = true;
            repo._tables_by_xml_name.insert(std::make_pair(xml_name, tc));
        }
    }
}

ts::PSIRepository::RegisterTable::RegisterTable(const std::vector<TID>& tids,
                                                Standards standards,
                                                DisplaySectionFunction display,
                                                LogSectionFunction log,
                                                std::initializer_list<PID> pids,
                                                CASID min_cas,
                                                CASID max_cas)
{
    // Use the complete constructor for actual registration.
    RegisterTable reg(nullptr, NullIndex(), tids, standards, UString(), display, log, pids, min_cas, max_cas);
}


//----------------------------------------------------------------------------
// Constructors to register a fully or partially implemented descriptor.
//----------------------------------------------------------------------------

ts::PSIRepository::RegisterDescriptor::RegisterDescriptor(DescriptorFactory factory,
                                                          std::type_index index,
                                                          const EDID& edid,
                                                          const UString& xml_name,
                                                          DisplayDescriptorFunction display,
                                                          const UString& legacy_xml_name)
{
    CERR.log(2, u"registering descriptor <%s>", xml_name);
    PSIRepository& repo(PSIRepository::Instance());
    DescriptorClassPtr dc;

    // Search an existing entry.
    const auto bounds(repo._descriptors_by_xdid.equal_range(edid.xdid()));
    for (auto it = bounds.first; it != bounds.second; ++it) {
        if (it->second->edid == edid) {
            // Found a compatible entry.
            dc = it->second;
        }
    }

    // Build a new entry if none found.
    if (dc == nullptr) {
        dc = std::make_shared<DescriptorClass>();
        repo._descriptors_by_xdid.insert(std::make_pair(edid.xdid(), dc));
    }

    // Build a description for this descriptor.
    dc->index = index;
    dc->edid = edid;
    dc->factory = factory;
    dc->display = display;
    dc->xml_name = xml_name;

    // Store the descriptor description.
    repo._descriptors_by_type_index.insert(std::make_pair(index, dc));

    // Associate XML names with descriptor classes and allowed table ids.
    if (!xml_name.empty()) {
        repo._descriptors_by_xml_name.insert(std::make_pair(xml_name, dc));
    }
    if (!legacy_xml_name.empty()) {
        repo._descriptors_by_xml_name.insert(std::make_pair(legacy_xml_name, dc));
    }
    if (edid.isTableSpecific()) {
        for (TID tid : edid.tableIds()) {
            if (!xml_name.empty()) {
                repo._descriptor_tids.insert(std::make_pair(xml_name, tid));
            }
            if (!legacy_xml_name.empty()) {
                repo._descriptor_tids.insert(std::make_pair(legacy_xml_name, tid));
            }
        }
    }
}

ts::PSIRepository::RegisterDescriptor::RegisterDescriptor(DisplayCADescriptorFunction display, CASID min_cas, CASID max_cas)
{
    if (display != nullptr) {
        PSIRepository& repo(PSIRepository::Instance());
        do {
            repo._casid_descriptor_displays.insert(std::make_pair(min_cas, display));
        } while (min_cas++ < max_cas);
    }
}


//----------------------------------------------------------------------------
// Signalization classes.
//----------------------------------------------------------------------------

ts::PSIRepository::SignalizationClass::~SignalizationClass() {}
ts::PSIRepository::DescriptorClass::~DescriptorClass() {}
ts::PSIRepository::TableClass::~TableClass() {}

ts::Standards ts::PSIRepository::TableClass::getStandards() const
{
    return standards;
}

ts::Standards ts::PSIRepository::DescriptorClass::getStandards() const
{
    return edid.standards();
}

// Selection algorithm between two possible tables or descriptors with the same id or characteristics.
// See comment in header file about mixed ISDB-DVB compatibility.
template<class SIG>
    requires std::derived_from<SIG, ts::PSIRepository::SignalizationClass>
void AddCandidate(std::shared_ptr<SIG>& fallback, size_t& fallback_count, const std::shared_ptr<SIG>& candidate, ts::Standards env_standards)
{
    constexpr ts::Standards ISDB = ts::Standards::ISDB;
    constexpr ts::Standards DVB_ISDB = ts::Standards::DVB | ts::Standards::ISDB;
    const ts::Standards cand_standards = candidate->getStandards();

    // If there are only two fallbacks, one with ISDB and one with DVB, use the ISDB one
    // if ISDB is a current standard, otherwise use DVB.
    if (fallback == nullptr || ((fallback->getStandards() | cand_standards) & DVB_ISDB) != DVB_ISDB) {
        // DVB and ISDB are not common to the previous fallback and the candidate. Just add a candidate.
        fallback = candidate;
        fallback_count++;
    }
    else if ((bool(env_standards & ISDB) && bool(cand_standards & ISDB)) || (!(env_standards & ISDB) && !(cand_standards & ISDB))) {
        // The new candidate is ISDB in an ISDB context or DVB in a DVB-only context, replace the fallback without incrementing the count
        fallback = candidate;
    }
    // else: the new candidate is ISDB in a DVB-only context or DVB in an ISDB context, ignore it.
}


//----------------------------------------------------------------------------
// Get the description of a table class for a given table id and context.
//----------------------------------------------------------------------------

const ts::PSIRepository::TableClass& ts::PSIRepository::getTable(TID tid, const SectionContext& context) const
{
    // Try to find an exact match with standard and CAS id. Otherwise, will use a fallback once for same tid.
    TableClassPtr fallback = nullptr;
    size_t fallback_count = 0;

    const PID pid = context.getPID();
    const CASID cas = context.getCAS();
    const Standards standards = context.getStandards();

    // Range of iterators for all table classes matching the table id.
    const auto bounds(_tables_by_tid.equal_range(tid));

    // Look for an exact match.
    for (auto it = bounds.first; it != bounds.second; ++it) {
        const auto& tc(it->second);

        // Standard match: at least one standard of the table is current, or standard-agnostic table (Standards::NONE).
        const bool std_match = bool(standards & tc->standards) || tc->standards == Standards::NONE;

        // Standard compatibility: already standard match or the table is compatible with the current standards
        // (and will therefore add a new standard to the context).
        const bool std_compat = std_match || CompatibleStandards(standards | tc->standards);

        // CAS match: either a CAS is specified and is in range, or no CAS specified and CAS-agnostic table (all CASID_NULL).
        const bool cas_match = cas >= tc->min_cas && cas <= tc->max_cas;

        if (tc->pids.contains(pid) && std_compat) {
            // If the table is in a standard PID, this is an exact match.
            return *tc;
        }
        else if (std_match && cas_match) {
            // Found an exact match, no need to search further.
            return *tc;
        }
        else if (tc->min_cas == CASID_NULL) {
            // Not the right standard but a CAS-agnostic table or no CAS specified, use as potential fallback.
            AddCandidate(fallback, fallback_count, tc, standards);
        }
    }

    // If no exact match was found, use a fallback if there is only one (no ambiguity).
    if (fallback_count == 1) {
        return *fallback;
    }
    else {
        // Thread-safe init-safe static data pattern:
        static const TableClass null_table_class;
        return null_table_class;
    }
}


//----------------------------------------------------------------------------
// Search a descriptor class from its EDID.
//----------------------------------------------------------------------------

const ts::PSIRepository::DescriptorClass& ts::PSIRepository::getDescriptor(EDID edid) const
{
    // Thread-safe init-safe static data pattern:
    static const DescriptorClass null_descriptor_class;

    // Get the range of XDID entries for this family of descriptors.
    const auto bounds(_descriptors_by_xdid.equal_range(edid.xdid()));

    // If the bounds are equal, no element matches, unknown descritor.
    if (bounds.first == bounds.second) {
        return null_descriptor_class;
    }

    // If there is only one descriptor, use it without further analysis.
    auto next = bounds.first;
    if (++next == bounds.second) {
        return *bounds.first->second;
    }

    // If there are several descriptors, search for an exact EDID match.
    for (next = bounds.first; next != bounds.second; ++next) {
        if (next->second->edid == edid) {
            return *next->second;
        }
    }

    // Ambiguous descriptor.
    return null_descriptor_class;
}


//----------------------------------------------------------------------------
// Search a descriptor class from the context.
//----------------------------------------------------------------------------

const ts::PSIRepository::DescriptorClass& ts::PSIRepository::getDescriptor(XDID xdid, DescriptorContext& context) const
{
    // Thread-safe init-safe static data pattern:
    static const DescriptorClass null_descriptor_class;

    // Get the range of XDID entries for this family of descriptors.
    const auto bounds(_descriptors_by_xdid.equal_range(xdid));

    // If the bounds are equal, no element matches, unknown descritor.
    if (bounds.first == bounds.second) {
        return null_descriptor_class;
    }

    // Immediately get TID and standards from the context.
    const TID tid = context.getTableId();
    const Standards standards = context.getStandards();

    // Search PDS and REGID later, only if necessary, this is possibly a lengthy operation.
    std::optional<PDS> pds;
    std::optional<REGIDVector> regids;

    // Handle specific case: Unknown DVB private descriptor (unsupported in TSDuck, undocumented by the vendor, etc)
    // have DID >= 0x80 and not edid.isPrivateDVB(). If the DID matches another descriptor from a non-incompatible
    // standard (eg. ISDB), the unknown private descriptor can be erroneously interpreted as ISDB. So, when the DID
    // is >= 0x80 and a PDS is active, it must be a private descriptor (DVB or MPEG).
    bool should_be_private = false;
    if (xdid.did() >= 0x80) {
        // Fetch the PDS now.
        pds = context.getPDS();
        should_be_private = pds.value() != 0 && pds.value() != PDS_NULL;
    }

    // Find possible matches.
    DescriptorClassPtr match;
    size_t match_count = 0;
    auto next = bounds.first;
    for ( ; next != bounds.second; ++next) {
        const auto& dc(next->second);
        if ((dc->edid.isExtension() && dc->edid.xdid() == xdid) || dc->edid.matchTableSpecific(tid, standards)) {
            // Extension descriptor or table-specific descriptor for the table we use, we have a match.
            return *dc;
        }
        else if (dc->edid.isPrivateDVB()) {
            // Search the PDS only once and only if necessary.
            if (!pds.has_value()) {
                pds = context.getPDS();
            }
            // If the current PDS matches the required one, we have a match.
            if (pds.value() != 0 && pds.value() != PDS_NULL && pds.value() == dc->edid.pds()) {
                return *dc;
            }
        }
        else if (dc->edid.isPrivateMPEG()) {
            // Search the REGIDs only once and only if necessary.
            if (!regids.has_value()) {
                regids.emplace();
                context.getREGIDs(regids.value());
            }
            const auto& ids(regids.value());
            if (!ids.empty()) {
                // The most relevant REGIDS are at the end of the list.
                for (auto reg = ids.rbegin(); reg != ids.rend(); ++reg) {
                    if (dc->edid.regid() == *reg) {
                        return *dc;
                    }
                }
            }
        }
        else if (!should_be_private && dc->edid.matchRegularStandards(standards)) {
            // We match the standards for a regular descriptor, this is a possible match.
            AddCandidate(match, match_count, dc, standards);
        }
    }

    // No private descriptor found. If there is exactly one regular match, we keep it.
    // Otherwise, there is either nothing found or some ambiguity.
    return match_count == 1 ? *match : null_descriptor_class;
}


//----------------------------------------------------------------------------
// Get the description of a descriptor for a descriptor closs RTTI index.
//----------------------------------------------------------------------------

const ts::PSIRepository::DescriptorClass& ts::PSIRepository::getDescriptor(std::type_index index, TID tid, Standards standards) const
{
    const auto bounds(_descriptors_by_type_index.equal_range(index));
    if (bounds.first == bounds.second) {
        // Descriptor class not found.
        // Thread-safe init-safe static data pattern:
        static const DescriptorClass null_descriptor_class;
        return null_descriptor_class;
    }
    if (tid != TID_NULL) {
        for (auto next = bounds.first; next != bounds.second; ++next) {
            if (next->second->edid.matchTableSpecific(tid, standards)) {
                return *next->second; // exact match for a table-specific descriptor
            }
        }
    }
    // Return the first definition for the table (if there are more than one).
    return *bounds.first->second;
}


//----------------------------------------------------------------------------
// Get simple registered items.
//----------------------------------------------------------------------------

const ts::PSIRepository::TableClass& ts::PSIRepository::getTable(const UString& xml_name) const
{
    const auto it = xml_name.findSimilar(_tables_by_xml_name);
    if (it != _tables_by_xml_name.end()) {
        return *it->second;
    }
    else {
        // Thread-safe init-safe static data pattern:
        static const TableClass null_table_class;
        return null_table_class;
    }
}

const ts::PSIRepository::DescriptorClass& ts::PSIRepository::getDescriptor(const UString& xml_name) const
{
    const auto it = xml_name.findSimilar(_descriptors_by_xml_name);
    if (it != _descriptors_by_xml_name.end()) {
        return *it->second;
    }
    else {
        // Thread-safe init-safe static data pattern:
        static const DescriptorClass null_descriptor_class;
        return null_descriptor_class;
    }
}

ts::DisplayCADescriptorFunction ts::PSIRepository::getCADescriptorDisplay(CASID cas_id) const
{
    const auto it = _casid_descriptor_displays.find(cas_id);
    return it != _casid_descriptor_displays.end() ? it->second : nullptr;
}


//----------------------------------------------------------------------------
// Get the list of standards which are defined for a given table id.
//----------------------------------------------------------------------------

ts::Standards ts::PSIRepository::getTableStandards(TID tid, PID pid, Standards current_standards) const
{
    // Accumulate the common subset of all standards for this table id.
    std::optional<Standards> standards;

    // Accumulate the common subset of all standards for this table id in incorrect PID's.
    std::optional<Standards> standards_bad_pid;

    // Accumulate the common subset of all standards for this table id with incompatible standards.
    std::optional<Standards> standards_bad_std;

    const auto bounds(_tables_by_tid.equal_range(tid));
    for (auto it = bounds.first; it != bounds.second; ++it) {
        const auto& tc(*it->second);
        if (tc.pids.contains(pid)) {
            // We are in a standard PID for this table id, return the corresponding standards only.
            return tc.standards;
        }
        else if (!CompatibleStandards(current_standards | tc.standards)) {
            // The candidate table is incompatible with the current standards.
            standards_bad_std = standards_bad_std.has_value() ? (standards_bad_std.value() & tc.standards) : tc.standards;
        }
        else if (!tc.pids.empty() && pid != PID_NULL) {
            // This is a table with dedicated PID's but we are not in one of them => store separately.
            standards_bad_pid = standards_bad_pid.has_value() ? (standards_bad_pid.value() & tc.standards) : tc.standards;
        }
        else {
            standards = standards.has_value() ? (standards.value() & tc.standards) : tc.standards;
        }
    }

    if (standards.has_value()) {
        // Found one or more table ids with compatible standards in their correct PID. Keep them only.
        return standards.value();
    }
    else if (standards_bad_pid.has_value()) {
        // Otherwise, accept table ids with compatible standards but in a wrong PID.
        return standards_bad_pid.value();
    }
    else {
        // Finally, accept table ids with incompatible standards.
        return standards_bad_std.value_or(Standards::NONE);
    }
}


//----------------------------------------------------------------------------
// Check if a descriptor is allowed in a table.
//----------------------------------------------------------------------------

bool ts::PSIRepository::isDescriptorAllowed(const UString& desc_node_name, TID table_id) const
{
    auto it = desc_node_name.findSimilar(_descriptor_tids);
    if (it == _descriptor_tids.end()) {
        // Not a table-specific descriptor, allowed anywhere
        return true;
    }
    else {
        // Table specific descriptor, the table needs to be listed.
        do {
            if (table_id == it->second) {
                // The table is explicitly allowed.
                return true;
            }
        } while (++it != _descriptor_tids.end() && desc_node_name.similar(it->first));
        // The requested table if was not found.
        return false;
    }
}


//----------------------------------------------------------------------------
// Get the list of tables where a descriptor is allowed.
//----------------------------------------------------------------------------

ts::UString ts::PSIRepository::descriptorTables(const DuckContext& duck, const UString& desc_node_name) const
{
    auto it = desc_node_name.findSimilar(_descriptor_tids);
    UString result;

    while (it != _descriptor_tids.end() && desc_node_name.similar(it->first)) {
        if (!result.empty()) {
            result.append(u", ");
        }
        result.append(TIDName(duck, it->second, PID_NULL, CASID_NULL, NamesFlags::NAME | NamesFlags::HEXA));
        ++it;
    }

    return result;
}


//----------------------------------------------------------------------------
// Get all registered items of a given type.
//----------------------------------------------------------------------------

void ts::PSIRepository::getRegisteredTableIds(std::vector<TID>& ids) const
{
    ids.clear();
    TID previous = TID_NULL;
    for (const auto& it : _tables_by_tid) {
        // This is a multimap, the same key can be reused, use it once only.
        if (it.first != previous) {
            ids.push_back(it.first);
            previous = it.first;
        }
    }
}

void ts::PSIRepository::getRegisteredDescriptorIds(std::vector<EDID>& ids) const
{
    ids.clear();
    for (const auto& it : _descriptors_by_xdid) {
        ids.push_back(it.second->edid);
    }
}

void ts::PSIRepository::getRegisteredTableNames(UStringList& names) const
{
    names = MapKeysList(_tables_by_xml_name);
}

void ts::PSIRepository::getRegisteredDescriptorNames(UStringList& names) const
{
    names = MapKeysList(_descriptors_by_xml_name);
}

void ts::PSIRepository::getRegisteredTablesModels(UStringList& names) const
{
    names = _xml_extension_files;
}


//----------------------------------------------------------------------------
// List all supported tables.
//----------------------------------------------------------------------------

void ts::PSIRepository::listTables(std::ostream& out) const
{
    TextTable table;
    table.addColumn(1, u"TID");
    table.addColumn(2, u"XML");
    table.addColumn(3, u"Standards");
    table.addColumn(4, u"Name");

    // Loop on all known tables.
    for (const auto& it : _tables_by_tid) {
        const auto& tc(*it.second);
        if (!tc.xml_name.empty() && tc.index != NullIndex()) {
            // This table is supported by TSDuck.
            table.newLine();
            table.setCell(1, UString::Format(u"%X", it.first));
            table.setCell(2, u"<" + tc.xml_name + u">");
            table.setCell(3, StandardsNames(tc.standards));
            table.setCell(4, tc.display_name);
        }
    }
    table.output(out, TextTable::Headers::UNDERLINED, false, UString(), u"  ");
}


//----------------------------------------------------------------------------
// List all supported descriptors.
//----------------------------------------------------------------------------

void ts::PSIRepository::listDescriptors(std::ostream& out) const
{
    TextTable table;
    table.addColumn(1, u"DID");
    table.addColumn(2, u"XML");
    table.addColumn(3, u"Standards");
    table.addColumn(4, u"Name, context");

    // Loop on all known descriptors.
    for (const auto& it : _descriptors_by_xdid) {
        const auto& dc(*it.second);
        if (!dc.xml_name.empty() && dc.index != NullIndex()) {
            // This descriptor is supported by TSDuck.
            table.newLine();
            table.setCell(1, it.first.toString());
            table.setCell(2, u"<" + dc.xml_name + u">");
            table.setCell(3, StandardsNames(dc.edid.standards()));
            UString name(dc.display_name);
            if (dc.edid.isPrivateDual()) {
                name += u", MPEG and DVB private (" + REGIDName(dc.edid.privateId()) + u")";
            }
            else if (dc.edid.isPrivateMPEG()) {
                name += u", MPEG private (" + REGIDName(dc.edid.regid()) + u")";
            }
            else if (dc.edid.isPrivateDVB()) {
                name += u", DVB private (" + PDSName(dc.edid.pds()) + u")";
            }
            else if (dc.edid.isTableSpecific()) {
                const std::set<TID> tids(dc.edid.tableIds());
                DuckContext duck;
                duck.addStandards(dc.edid.standards());
                const UChar* prefix = u", only in ";
                for (TID tid : tids) {
                    name += prefix;
                    name += TIDName(duck, tid);
                    prefix = u", ";
                }
            }
            table.setCell(4, name);
        }
    }
    table.output(out, TextTable::Headers::UNDERLINED, false, UString(), u"  ");
}


//----------------------------------------------------------------------------
// Dump the internal state of the PSI repository (for debug only).
//----------------------------------------------------------------------------

void ts::PSIRepository::dumpInternalState(std::ostream& out) const
{
    out << "TSDuck PSI Repository" << std::endl
        << "=====================" << std::endl
        << std::endl
        << "==== TID to table class: " << _tables_by_tid.size() << std::endl << std::endl;

    TextTable table;
    table.addColumn(1, u"TID");
    table.addColumn(2, u"Name");
    table.addColumn(3, u"XML");
    table.addColumn(4, u"Standards");
    table.addColumn(5, u"Class");
    table.addColumn(6, u"PID");
    table.addColumn(7, u"CAS");

    for (const auto& it : _tables_by_tid) {
        const auto& tc(*it.second);
        table.newLine();
        table.setCell(1, UString::Format(u"%X", it.first));
        table.setCell(2, NameToString(u"'", tc.display_name, u"'"));
        table.setCell(3, NameToString(u"<", tc.xml_name, u">"));
        table.setCell(4, StandardsToString(tc.standards));
        table.setCell(5, TypeIndexToString(tc.index));
        table.setCell(6, PIDsToString(tc.pids));
        table.setCell(7, CASToString(tc.min_cas, tc.max_cas));
    }
    table.output(out, TextTable::Headers::UNDERLINED);

    out << std::endl << "==== Table XML name to table class: " << _tables_by_xml_name.size() << std::endl << std::endl;
    table.clear();
    table.addColumn(1, u"XML");
    table.addColumn(2, u"Class");

    for (const auto& it : _tables_by_xml_name) {
        table.newLine();
        table.setCell(1, NameToString(u"<", it.first, u">"));
        table.setCell(2, TypeIndexToString(it.second->index));
    }
    table.output(out, TextTable::Headers::UNDERLINED);

    out << std::endl << "==== XDID to descriptor class: " << _descriptors_by_xdid.size() << std::endl << std::endl;
    table.clear();
    table.addColumn(1, u"XDID");
    table.addColumn(2, u"Name");
    table.addColumn(3, u"XML");
    table.addColumn(4, u"EDID");
    table.addColumn(5, u"Class");

    for (const auto& it : _descriptors_by_xdid) {
        const auto& dc(*it.second);
        table.newLine();
        table.setCell(1, it.first.toString());
        table.setCell(2, NameToString(u"'", dc.display_name, u"'"));
        table.setCell(3, NameToString(u"<", dc.xml_name, u">"));
        table.setCell(4, dc.edid.toString());
        table.setCell(5, TypeIndexToString(dc.index));
    }
    table.output(out, TextTable::Headers::UNDERLINED);

    out << std::endl << "==== Descriptor name to descriptor class: " << _descriptors_by_xml_name.size() << std::endl << std::endl;
    table.clear();
    table.addColumn(1, u"XML");
    table.addColumn(2, u"Class");

    for (const auto& it : _descriptors_by_xml_name) {
        table.newLine();
        table.setCell(1, NameToString(u"<", it.first, u">"));
        table.setCell(2, TypeIndexToString(it.second->index));
    }
    table.output(out, TextTable::Headers::UNDERLINED);

    out << std::endl << "==== Descriptor RTTI index to descriptor class: " << _descriptors_by_type_index.size() << std::endl << std::endl;
    table.clear();
    table.addColumn(1, u"Class");
    table.addColumn(2, u"Name");
    table.addColumn(3, u"XML");

    for (const auto& it : _descriptors_by_type_index) {
        table.newLine();
        table.setCell(1, TypeIndexToString(it.first));
        table.setCell(2, NameToString(u"'", it.second->display_name, u"'"));
        table.setCell(3, NameToString(u"<", it.second->xml_name, u">"));
    }
    table.output(out, TextTable::Headers::UNDERLINED);

    out << std::endl << "==== XML descriptor name to table id for table-specific descriptors: " << _descriptor_tids.size() << std::endl << std::endl;
    table.clear();
    table.addColumn(1, u"XML");
    table.addColumn(2, u"TID");

    for (const auto& it : _descriptor_tids) {
        table.newLine();
        table.setCell(1, NameToString(u"<", it.first, u">"));
        table.setCell(2, UString::Format(u"%X", it.second));
    }
    table.output(out, TextTable::Headers::UNDERLINED);

    out << std::endl << "==== Display CA Descriptor functions: " << _casid_descriptor_displays.size() << std::endl << std::endl;
    table.clear();
    table.addColumn(1, u"CASID");
    table.addColumn(2, u"Display function");

    for (const auto& it : _casid_descriptor_displays) {
        table.newLine();
        table.setCell(1, UString::Format(u"%X", it.first));
        table.setCell(2, UString::Format(u"%X", size_t(it.second)));
    }
    table.output(out, TextTable::Headers::UNDERLINED);

    out << std::endl << "==== XML extension files: " << _xml_extension_files.size() << std::endl << std::endl;
    for (const auto& it : _xml_extension_files) {
        out << "\"" << it << "\"" << std::endl;
    }
    out << std::endl;
}


//----------------------------------------------------------------------------
// Display utilities.
//----------------------------------------------------------------------------

ts::UString ts::PSIRepository::NameToString(const UString& prefix, const UString& name, const UString& suffix)
{
    return name.empty() ? u"-" : prefix + name + suffix;
}

ts::UString ts::PSIRepository::TypeIndexToString(std::type_index index)
{
    if (index == NullIndex()) {
        return u"-";
    }
    else {
        const UString name(ClassName(index));
        return name.empty() ? UString::Format(u"%X", index.hash_code()): name;
    }
}

ts::UString ts::PSIRepository::StandardsToString(Standards std)
{
    return std == Standards::NONE ? u"-" : StandardsNames(std);
}

ts::UString ts::PSIRepository::PIDsToString(const std::set<PID>& pids)
{
    if (pids.empty()) {
        return u"-";
    }
    else {
        UString s;
        for (auto pid : pids) {
            if (!s.empty()) {
                s.append(u", ");
            }
            s.format(u"%X", pid);
        }
        return s;
    }
}

ts::UString ts::PSIRepository::CASToString(CASID min, CASID max)
{
    return min == CASID_NULL ? u"-" : (min == max ? UString::Format(u"%X", min) : UString::Format(u"%X-%X", min, max));
}
