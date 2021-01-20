//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2021, Thierry Lelegard
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

#include "tsxmlJSONConverterArgs.h"
#include "tsArgs.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::xml::JSONConverterArgs::JSONConverterArgs() :
    include_root(false),
    enforce_integer(false),
    enforce_boolean(false),
    trim_text(false),
    collapse_text(false)
{
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::xml::JSONConverterArgs::defineArgs(Args& args) const
{
    args.option(u"x2j-include-root");
    args.help(u"x2j-include-root",
              u"In the XML-to-JSON conversion, keep the root of the XML document as a JSON object. "
              u"By default, the JSON document is made of an array of all XML elements under the root.");

    args.option(u"x2j-enforce-integer");
    args.help(u"x2j-enforce-integer",
              u"In the XML-to-JSON conversion, when an element attribute contains an integer value "
              u"but there is no XML model file to tell if this is really an integer, force the creation "
              u"of a JSON number. By default, when there is no XML model, all element attributes are "
              u"converted as JSON strings.");

    args.option(u"x2j-enforce-boolean");
    args.help(u"x2j-enforce-boolean",
              u"In the XML-to-JSON conversion, when an element attribute contains a boolean value "
              u"but there is no XML model file to tell if this is really a boolean, force the creation "
              u"of a JSON boolean. By default, when there is no XML model, all element attributes are "
              u"converted as JSON strings.");

    args.option(u"x2j-trim-text");
    args.help(u"x2j-trim-text",
              u"In the XML-to-JSON conversion, remove leading and trailing spaces in all text nodes. "
              u"By default, text nodes are trimmed only when there is an XML model which identifies "
              u"the text node as containing hexadecimal content.");

    args.option(u"x2j-collapse-text");
    args.help(u"x2j-collapse-text",
              u"In the XML-to-JSON conversion, remove leading and trailing spaces and replace all other "
              u"sequences of space characters by one single space in all text nodes. "
              u"By default, text nodes are collapsed only when there is an XML model which identifies "
              u"the text node as containing hexadecimal content.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
// Args error indicator is set in case of incorrect arguments
//----------------------------------------------------------------------------

bool ts::xml::JSONConverterArgs::loadArgs(DuckContext& duck, Args& args)
{
    include_root = args.present(u"x2j-include-root");
    enforce_integer = args.present(u"x2j-enforce-integer");
    enforce_boolean = args.present(u"x2j-enforce-boolean");
    collapse_text = args.present(u"x2j-collapse-text");
    trim_text = collapse_text || args.present(u"x2j-trim-text");
    return true;
}
