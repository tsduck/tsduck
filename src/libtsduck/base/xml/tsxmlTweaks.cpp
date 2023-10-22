//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsxmlTweaks.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::xml::Tweaks::defineArgs(Args& args)
{
    args.option(u"strict-xml");
    args.help(u"strict-xml",
              u"Save XML documents in strictly conformant XML format. "
              u"By default, do not escape characters when this is not syntactically "
              u"necessary to make the XML text more human-readable.");

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
//----------------------------------------------------------------------------

bool ts::xml::Tweaks::loadArgs(DuckContext& duck, Args& args)
{
    attributeValueDoubleQuote = true;
    strictAttributeFormatting = true;
    strictTextNodeFormatting = args.present(u"strict-xml");
    x2jIncludeRoot = args.present(u"x2j-include-root");
    x2jEnforceInteger = args.present(u"x2j-enforce-integer");
    x2jEnforceBoolean = args.present(u"x2j-enforce-boolean");
    x2jCollapseText = args.present(u"x2j-collapse-text");
    x2jTrimText = x2jCollapseText || args.present(u"x2j-trim-text");
    return true;
}
