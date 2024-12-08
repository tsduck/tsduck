//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsTID.h"
#include "tsDuckContext.h"


//----------------------------------------------------------------------------
// Name of a Table ID.
//----------------------------------------------------------------------------

ts::UString ts::TIDName(const DuckContext& duck, TID tid, CASID cas, NamesFlags flags)
{
    // Where to search table ids.
    const NamesFile::NamesFilePtr repo = NamesFile::Instance(NamesFile::Predefined::DTV);
    const UString section(u"TableId");
    const NamesFile::Value cas_value = NamesFile::Value(CASFamilyOf(cas)) << 8;
    const NamesFile::Value tid_value = NamesFile::Value(tid);

    if (repo->nameExists(section, tid_value | cas_value)) {
        // TID+CAS is found, without standard.
        return repo->nameFromSection(section, tid_value | cas_value, flags);
    }
    else if (repo->nameExists(section, tid_value)) {
        // TID alone found without standard, without CAS. Keep this value.
        return repo->nameFromSection(section, tid_value, flags);
    }
    else {
        // Loop on all possible standards. Build a list of possible names.
        UStringList all_names;
        bool found_with_standard = false;
        for (Standards mask = Standards(1); mask != Standards::NONE; mask <<= 1) {
            // Check if this standard is currently in TSDuck context.
            const bool has_standard = bool(duck.standards() & mask);
            const NamesFile::Value std_value = NamesFile::Value(mask) << 16;
            // Lookup name only if supported standard or no previous standard was found.
            if (!found_with_standard || has_standard) {
                UString name;
                if (repo->nameExists(section, tid_value | std_value | cas_value)) {
                    // Found with that standard and CAS.
                    name = repo->nameFromSection(section, tid_value | std_value | cas_value, flags);
                }
                else if (repo->nameExists(section, tid_value | std_value)) {
                    // Found with that standard, without CAS.
                    name = repo->nameFromSection(section, tid_value | std_value, flags);
                }
                if (!name.empty()) {
                    // A name was found.
                    if (!found_with_standard && has_standard) {
                        // At least one supported standard is found.
                        // Clear previous results without supported standard.
                        // Will no longer try without supported standard.
                        found_with_standard = true;
                        all_names.clear();
                    }
                    all_names.push_back(name);
                }
            }
        }
        if (all_names.empty()) {
            // No name found, use default formatting with the value only.
            return repo->nameFromSection(section, tid_value, flags);
        }
        else {
            // One or more possibility. Return them all since we cannot choose.
            return UString::Join(all_names, u" or ");
        }
    }
}
