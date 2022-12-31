//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
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
    return new ts::DuckContext(rep == nullptr ? ts::NullReport::Instance() : rep);
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
        duck->setTimeReferenceOffset(ts::MilliSecond(offset));
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
