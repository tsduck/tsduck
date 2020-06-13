// Sample TSDuck extension.
// Definition of a section filter for ts::TablesLogger.

#include "fooFilter.h"

// Register this section filter in the reposity.
TS_REGISTER_SECTION_FILTER(foo::FooFilter);


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

foo::FooFilter::FooFilter() :
    _negate_id(false),
    _ids()
{
}


//----------------------------------------------------------------------------
// Define section filtering command line options in an Args.
//----------------------------------------------------------------------------

void foo::FooFilter::defineFilterOptions(ts::Args& args) const
{
    args.option(u"foo-id", 0, ts::Args::UINT16, 0, ts::Args::UNLIMITED_COUNT);
    args.help(u"foo-id", u"id1[-id2]",
              u"Select FOOT, ECM or EMM sections with this 'foo_id' value or range of values. "
              u"Several --foo-id options may be specified.");

    args.option(u"negate-foo-id");
    args.help(u"negate-foo-id",
              u"Negate the 'foo_id' filter: sections with the specified id's are excluded.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool foo::FooFilter::loadFilterOptions(ts::DuckContext& duck, ts::Args& args, ts::PIDSet& initial_pids)
{
    // Load options from the command line.
    _negate_id = args.present(u"negate-foo-id");
    args.getIntValues(_ids, u"foo-id");
    return true;
}


//----------------------------------------------------------------------------
// Check if a specific section must be filtered and displayed.
// This function must return true if the section can be displayed and false
// if it must not be displayed. A section is actually displayed if all section
// filters returned true.
//----------------------------------------------------------------------------

bool foo::FooFilter::filterSection(ts::DuckContext& duck, const ts::Section& section, uint16_t cas, ts::PIDSet& more_pids)
{
    // If we have no filtering criteria set, then let other filters decide.
    if (_ids.empty()) {
        return true;
    }

    // At this point, we must have a matching 'foo_id' in the section.
    // A 'foo_id' can be present in a FOOT or in an ECM or EMM coming from FooCAS.
    const ts::TID tid = section.tableId();
    const bool isECMM = tid >= ts::TID_CAS_FIRST && tid <= ts::TID_CAS_LAST;
    const bool isFooCAS = cas >= CASID_FOO_MIN && cas <= CASID_FOO_MAX;
    if (tid != TID_FOOT && (!isECMM || !isFooCAS)) {
        // There is no 'foo_id' at all in those sections.
        return false;
    }

    // Get the foo_id value.
    uint16_t foo_id = 0;
    if (tid == TID_FOOT) {
        // In a FOOT, the foo_id is in the tid-ext.
        foo_id = section.tableIdExtension();
    }
    else if (section.payloadSize() < 2) {
        // FooCAS ECM or EMM but too short, invalid, don't display.
        return false;
    }
    else {
        // FooCAS ECM or EMM, the foo_id is in the first two bytes of the ECM/EMM payload.
        foo_id = ts::GetUInt16(section.payload());
    }

    // Is this a selected foo_id?
    const bool id_set = _ids.find(foo_id) != _ids.end();

    // The section may be displayed if the value matches (or the opposite with --negate-foo-id).
    return (id_set && !_negate_id) || (!id_set && _negate_id);
}
