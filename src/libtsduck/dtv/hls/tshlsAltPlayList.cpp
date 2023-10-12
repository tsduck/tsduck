//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tshlsAltPlayList.h"


//----------------------------------------------------------------------------
// Implementation of StringifyInterface
//----------------------------------------------------------------------------

ts::UString ts::hls::AltPlayList::toString() const
{
    UString str(MediaElement::toString());

    if (!type.empty()) {
        str.format(u", type: %s", {type});
    }
    if (!name.empty()) {
        str.format(u", name: %s", {name});
    }
    if (!groupId.empty()) {
        str.format(u", group id: %s", {groupId});
    }
    if (!language.empty()) {
        str.format(u", language: %s", {language});
    }

    return str;
}
