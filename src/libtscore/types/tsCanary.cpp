//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsCanary.h"
#include "tsUString.h"


//----------------------------------------------------------------------------
// Check that a Canary pointer is valid and get an error message.
//----------------------------------------------------------------------------

const ts::UChar* ts::Canary::Error(const Canary* c)
{
    if (c == nullptr) {
        return u"null pointer";
    }
    else {
        // Attempt memory access.
        try {
            if (c->_canary != GOOD) {
                return c->_canary == BAD ? u"reuse after free" : u"not a canary block";
            }
        }
        catch (...) {
            return u"invalid address";
        }
    }
    return nullptr;
}


//----------------------------------------------------------------------------
// Check that a Canary pointer is valid, get and log an error message.
//----------------------------------------------------------------------------

const ts::UChar* ts::Canary::LogError(const Canary* c)
{
    const UChar* err = Error(c);
    if (err != nullptr) {
        // Save cerr format state, restore later.
        std::ios cerr_state(nullptr);
        cerr_state.copyfmt(std::cerr);
        std::cerr << std::endl << "**** internal error: invalid memory canary, address: "
                  << std::hex << std::setw(2 * sizeof(c)) << std::setfill('0') << uintptr_t(c)
                  << ", " << err << std::endl << std::flush;
        std::cerr.copyfmt(cerr_state);
    }
    return err;
}
