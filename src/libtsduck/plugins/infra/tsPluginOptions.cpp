//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
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
