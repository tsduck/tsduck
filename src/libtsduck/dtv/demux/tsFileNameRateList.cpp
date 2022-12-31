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

#include "tsFileNameRateList.h"
#include "tsxmlDocument.h"


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

bool ts::FileNameRateList::getArgs(Args& args, const UChar* option_name, MilliSecond default_rate)
{
    // Get the string values
    UStringVector strings;
    args.getValues(strings, option_name);

    // Decode the args
    clear();
    bool success = true;

    for (size_t i = 0; i < strings.size(); ++i) {
        const UString::size_type eq = strings[i].find('=');
        FileNameRate file;
        file.repetition = default_rate;
        file.inline_xml = xml::Document::IsInlineXML(strings[i]);
        if (file.inline_xml || eq == UString::npos) {
            // No '=' found, no repetition rate specified.
            file.file_name = strings[i];
        }
        else {
            // A repetition rate is specified after '='.
            file.file_name = strings[i].substr(0, eq);
            if (!strings[i].substr(eq + 1).toInteger(file.repetition) || file.repetition <= 0) {
                args.error(u"invalid repetition rate for file " + file.file_name);
                success = false;
            }
        }
        file.display_name = xml::Document::DisplayFileName(file.file_name);
        push_back(file);
    }

    return success;
}
