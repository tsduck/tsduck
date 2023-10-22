//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsBoolPredicate.h"

bool ts::MultiAnd(std::initializer_list<bool> args)
{
    for (auto it : args) {
        if (!it) {
            return false;
        }
    }
    return args.size() > 0;
}

bool ts::MultiOr(std::initializer_list<bool> args)
{
    for (auto it : args) {
        if (it) {
            return true;
        }
    }
    return false;
}
