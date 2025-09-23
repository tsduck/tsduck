//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsFileNameRateList.h"
#include "tsxmlDocument.h"
#include "tsjson.h"


//----------------------------------------------------------------------------
// Scan the files for update.
//----------------------------------------------------------------------------

size_t ts::FileNameRateList::scanFiles(size_t retry, Report& report)
{
    size_t count = 0;
    for (auto it = begin(); it != end(); ++it) {
        if (it->scanFile(retry, report)) {
            ++count;
        }
    }
    return count;
}


//----------------------------------------------------------------------------
// Decode a list of parameters containing a list of file names with
// optional repetition rates in milliseconds.
//----------------------------------------------------------------------------

bool ts::FileNameRateList::getArgs(Args& args, const UChar* option_name, cn::milliseconds default_rate)
{
    // Get the string values
    UStringVector strings;
    args.getValues(strings, option_name);

    // Decode the args
    clear();
    bool success = true;

    for (size_t i = 0; i < strings.size(); ++i) {
        FileNameRate file;
        file.repetition = default_rate;
        file.inline_xml = xml::Document::IsInlineXML(strings[i]);
        file.inline_json = json::IsInlineJSON(strings[i]);
        const UString::size_type eq = strings[i].find('=');
        if (file.inline_xml || file.inline_json || eq == UString::npos) {
            // No '=' found after file name, no repetition rate specified.
            file.file_name = strings[i];
        }
        else {
            // A repetition rate is specified after '='.
            file.file_name = strings[i].substr(0, eq);
            if (!strings[i].substr(eq + 1).toChrono(file.repetition) || file.repetition <= cn::milliseconds::zero()) {
                args.error(u"invalid repetition rate for file " + file.file_name);
                success = false;
            }
        }
        file.display_name = xml::Document::DisplayFileName(file.file_name);
        push_back(file);
    }

    return success;
}
