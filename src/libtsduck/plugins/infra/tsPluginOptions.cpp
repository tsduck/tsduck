//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsPluginOptions.h"

ts::PluginOptions::PluginOptions(const ts::UString& name_, const UStringVector& args_) :
    name(name_),
    args(args_)
{
}

void ts::PluginOptions::set(const ts::UString& name_, const UStringVector& args_)
{
    name = name_;
    args = args_;
}

void ts::PluginOptions::clear()
{
    name.clear();
    args.clear();
}

ts::UString ts::PluginOptions::toString(PluginType type) const
{
    if (name.empty()) {
        return UString();
    }
    UString str;
    switch (type) {
        case PluginType::INPUT:
            str = u"-I ";
            break;
        case PluginType::OUTPUT:
            str = u"-O ";
            break;
        case PluginType::PROCESSOR:
            str = u"-P ";
            break;
        default:
            break;
    }
    str.append(name);
    for (const auto& it : args) {
        str.append(u" ");
        str.append(it.toQuoted());
    }
    return str;
}
