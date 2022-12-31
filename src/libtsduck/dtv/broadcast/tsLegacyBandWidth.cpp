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

#include "tsLegacyBandWidth.h"
#include "tsxmlElement.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Convert a string containing a bandwidth value into an integer value in Hz.
//----------------------------------------------------------------------------

bool ts::LegacyBandWidthToHz(BandWidth& hz, const UString& str)
{
    // Redefine legacy names with values in Hz.
    const Enumeration legacy({
        {u"auto",      0},
        {u"1.712-MHz", 1712000},
        {u"5-MHz",     5000000},
        {u"6-MHz",     6000000},
        {u"7-MHz",     7000000},
        {u"8-MHz",     8000000},
        {u"10-MHz",    10000000},
    });

    int bw = legacy.value(str, false);
    if (bw != Enumeration::UNKNOWN) {
        // Found a legacy value.
        hz = BandWidth(bw);
        return true;
    }
    else if (!str.toInteger(bw, u",") || bw < 0) {
        // Not a positive integer and not a legacy value.
        return false;
    }
    else if (bw < 1000) {
        // Low values, less than 1000, are interpreted in MHz (legacy again...)
        hz = BandWidth(bw * 1000000);
        return true;
    }
    else {
        // Actual value in Hz.
        hz = BandWidth(bw);
        return true;
    }
}


//----------------------------------------------------------------------------
// Get optional bandwidth parameter from XML element.
//----------------------------------------------------------------------------

bool ts::GetLegacyBandWidth(Variable<BandWidth>& bandwidth, const xml::Element* element, const UString& attribute)
{
    BandWidth bw = 0;

    // Get attibute as a string
    UString str;
    element->getAttribute(str, attribute);

    if (str.empty()) {
        // Attribute not present, ok.
        bandwidth.clear();
        return true;
    }
    else if (LegacyBandWidthToHz(bw, str)) {
        // Valid value.
        bandwidth = bw;
        return true;
    }
    else {
        element->report().error(u"'%s' is not a valid value for attribute '%s' in <%s>, line %d", {str, attribute, element->name(), element->lineNumber()});
        bandwidth.clear();
        return false;
    }
}


//----------------------------------------------------------------------------
// Add a command line option definition for bandwidth.
//----------------------------------------------------------------------------

void ts::DefineLegacyBandWidthArg(Args& args, const UChar* name, UChar short_name, BandWidth dvbt_default, BandWidth isdbt_default)
{
    UString help(u"Bandwidth in Hz. For compatibility with old versions, low values (below 1000) are interpreted in MHz.");

    if (dvbt_default != 0 || isdbt_default != 0) {
        help.append(u" The default is ");
        if (dvbt_default != 0) {
            help.format(u"%'d for DVB-T/T2", {dvbt_default});
        }
        if (dvbt_default != 0 && isdbt_default != 0) {
            help.append(u" and ");
        }
        if (isdbt_default != 0) {
            help.format(u"%'d for ISDB-T", {isdbt_default});
        }
        help.append(u".");
    }

    args.option(name, short_name, Args::STRING);
    args.help(name, help);
}

//----------------------------------------------------------------------------
// Load a bandwidth argument from command line.
//----------------------------------------------------------------------------

bool ts::LoadLegacyBandWidthArg(BandWidth& bandwidth, Args& args, const UChar* name, BandWidth def_value)
{
    const UString str(args.value(name));
    if (str.empty()) {
        bandwidth = def_value;
        return true;
    }
    else if (LegacyBandWidthToHz(bandwidth, str)) {
        return true;
    }
    else {
        args.error(u"invalid value '%s' for --%s", {str, name});
        bandwidth = def_value;
        return false;
    }
}

bool ts::LoadLegacyBandWidthArg(Variable<BandWidth>& bandwidth, Args& args, const UChar* name)
{
    BandWidth bw = 0;
    const UString str(args.value(name));
    if (str.empty()) {
        bandwidth.clear();
        return true;
    }
    else if (LegacyBandWidthToHz(bw, str)) {
        bandwidth = bw;
        return true;
    }
    else {
        args.error(u"invalid value '%s' for --%s", {str, name});
        bandwidth.clear();
        return false;
    }
}
