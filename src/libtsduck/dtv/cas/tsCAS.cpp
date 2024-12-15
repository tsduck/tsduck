//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCAS.h"
#include "tsDuckContext.h"
#include "tsCerrReport.h"


//----------------------------------------------------------------------------
// CAS families and ranges.
//----------------------------------------------------------------------------

namespace {

    // A repository of all CAS.
    class CASRepository : private ts::NamesFile::Visitor
    {
    public:
        // Constructor.
        CASRepository();

        // Proxy functions for public interface.
        ts::CASFamily CASFamilyOf(ts::CASID casid) const;
        bool GetCASIdRange(ts::CASFamily cas, ts::CASID& min, ts::CASID& max) const;

        // Implementation of NamesFile::Visitor.
        virtual bool handleNameValue(const ts::UString& section_name, ts::NamesFile::Value value, const ts::UString& name) override;

    private:
        struct CASDesc {
            ts::CASFamily family = ts::CAS_OTHER;
            ts::CASID     min = ts::CASID_NULL;
            ts::CASID     max = ts::CASID_NULL;
        };

        mutable std::mutex _mutex {};
        std::list<CASDesc> _cas {};
    };
}

TS_STATIC_INSTANCE(, CASRepository, AllCAS, ());


//----------------------------------------------------------------------------
// Fill the CAS repository from names files.
//----------------------------------------------------------------------------

// Constructor.
CASRepository::CASRepository()
{
    CERR.debug(u"creating PSIRepository");

    // Load all CAS ranges from a names file and subscribe to further updates.
    const auto repo = ts::NamesFile::Instance(ts::NamesFile::Predefined::DTV);
    repo->visitSection(this, u"CASFamilyRange");
    repo->subscribe(this, u"CASFamilyRange");

}

// Get a range of CAS ids.
bool CASRepository::handleNameValue(const ts::UString& section_name, ts::NamesFile::Value value, const ts::UString& name)
{
    // Cleanup name: remove comments and spaces.
    ts::UString range(name);
    const size_t dash = range.find(u'#');
    if (dash < range.length()) {
        range.resize(dash);
    }
    range.trim();

    // Extract "min-max" range.
    CASDesc cas;
    cas.family = ts::CASFamily(value);
    if (value < 0x100 && range.scan(u"%d-%d", &cas.min, &cas.max)) {
        std::lock_guard<std::mutex> lock(_mutex);
        _cas.push_back(cas);
    }
    else {
        CERR.error(u"invalid CAS family range in configuration file: 0x%02X = %s", value, name);
    }
    return true; // continue visting other values.
}


//----------------------------------------------------------------------------
// Return a CAS family from a CA system id.
//----------------------------------------------------------------------------

ts::CASFamily ts::CASFamilyOf(CASID casid)
{
    return AllCAS->CASFamilyOf(casid);
}

ts::CASFamily CASRepository::CASFamilyOf(ts::CASID casid) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& c : _cas) {
        if (casid >= c.min && casid <= c.max) {
            return c.family;
        }
    }
    return ts::CAS_OTHER;
}


//----------------------------------------------------------------------------
// Get the minimum and maximum CA system id in a CAS family.
//----------------------------------------------------------------------------


bool ts::GetCASIdRange(CASFamily cas, CASID& min, CASID& max)
{
    return AllCAS->GetCASIdRange(cas, min, max);
}

ts::CASID ts::FirstCASId(CASFamily cas)
{
    CASID min, max;
    AllCAS->GetCASIdRange(cas, min, max);
    return min;
}

ts::CASID ts::LastCASId(CASFamily cas)
{
    CASID min, max;
    AllCAS->GetCASIdRange(cas, min, max);
    return max;
}

bool CASRepository::GetCASIdRange(ts::CASFamily cas, ts::CASID& min, ts::CASID& max) const
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (const auto& c : _cas) {
        if (cas == c.family) {
            min = c.min;
            max = c.max;
            return true;
        }
    }
    min = max = ts::CASID_NULL;
    return false;
}


//----------------------------------------------------------------------------
// Name of Conditional Access Families.
//----------------------------------------------------------------------------

ts::UString ts::CASFamilyName(CASFamily cas)
{
    return NameFromDTV(u"CASFamily", cas, NamesFlags::NAME | NamesFlags::DECIMAL);
}


//----------------------------------------------------------------------------
// Name of a Conditional Access System Id (as in CA Descriptor).
//----------------------------------------------------------------------------

ts::UString ts::CASIdName(const DuckContext& duck, uint16_t casid, NamesFlags flags)
{
    // In the case of ISDB, look into another table (but only known names).
    if (bool(duck.standards() & Standards::ISDB)) {
        const UString name(NameFromDTV(u"ARIBCASystemId", NamesFile::Value(casid), flags | NamesFlags::NO_UNKNOWN));
        if (!name.empty()) {
            return name;
        }
    }

    // Not ISDB or not found in ISDB, use standard CAS names.
    return NameFromDTV(u"CASystemId", NamesFile::Value(casid), flags);
}
