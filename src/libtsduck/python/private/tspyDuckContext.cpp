//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//
//  TSDuck Python bindings: encapsulates DuckContext objects for Python.
//
//----------------------------------------------------------------------------

#include "tspy.h"
#include "tsDuckContext.h"
#include "tsNullReport.h"

TSDUCKPY void* tspyNewDuckContext(void* report)
{
    ts::Report* rep = reinterpret_cast<ts::Report*>(report);
    return new ts::DuckContext(rep == nullptr ? &NULLREP : rep);
}

TSDUCKPY void tspyDeleteDuckContext(void* duck_ptr)
{
    delete reinterpret_cast<ts::DuckContext*>(duck_ptr);
}

TSDUCKPY bool tspyDuckContextSetDefaultCharset(void* duck_ptr, const uint8_t* name, size_t name_size)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    if (duck != nullptr) {
        const ts::UString str(ts::py::ToString(name, name_size));
        const ts::Charset* charset = ts::Charset::GetCharset(str);
        if (charset != nullptr) {
            duck->setDefaultCharsetIn(charset);
            duck->setDefaultCharsetOut(charset);
            return true;
        }
        duck->report().error(u"unknown character set \"%s\"", {str});
    }
    return false;
}

TSDUCKPY void tspyDuckContextSetDefaultCASId(void* duck_ptr, uint16_t cas)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    if (duck != nullptr) {
        duck->setDefaultCASId(cas);
    }
}

TSDUCKPY void tspyDuckContextSetDefaultPDS(void* duck_ptr, uint32_t pds)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    if (duck != nullptr) {
        duck->setDefaultPDS(pds);
    }
}

TSDUCKPY void tspyDuckContextAddStandards(void* duck_ptr, uint32_t mask)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    if (duck != nullptr) {
        duck->addStandards(ts::Standards(mask));
    }
}

TSDUCKPY void tspyDuckContextResetStandards(void* duck_ptr, uint32_t mask)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    if (duck != nullptr) {
        duck->resetStandards(ts::Standards(mask));
    }
}

TSDUCKPY uint32_t tspyDuckContextStandards(void* duck_ptr)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    return duck == nullptr ? 0 : uint32_t(duck->standards());
}

TSDUCKPY void tspyDuckContextSetTimeReferenceOffset(void* duck_ptr, int64_t offset)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    if (duck != nullptr) {
        duck->setTimeReferenceOffset(cn::milliseconds(cn::milliseconds::rep(offset)));
    }
}

TSDUCKPY bool tspyDuckContextSetTimeReference(void* duck_ptr, const uint8_t* name, size_t name_size)
{
    ts::DuckContext* duck = reinterpret_cast<ts::DuckContext*>(duck_ptr);
    if (duck != nullptr) {
        const ts::UString str(ts::py::ToString(name, name_size));
        if (duck->setTimeReference(str)) {
            return true;
        }
        duck->report().error(u"invalid time reference \"%s\"", {str});
    }
    return false;
}
