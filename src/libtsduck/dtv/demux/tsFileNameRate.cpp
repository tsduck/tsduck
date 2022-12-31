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

#include "tsFileNameRate.h"
#include "tsxmlDocument.h"
#include "tsFileUtils.h"


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::FileNameRate::FileNameRate(const UString& name, MilliSecond rep) :
    file_name(name),
    display_name(xml::Document::DisplayFileName(file_name)),
    inline_xml(xml::Document::IsInlineXML(file_name)),
    file_date(),
    repetition(rep) ,
    retry_count(1)
{
}


//----------------------------------------------------------------------------
// Comparison operators.
//----------------------------------------------------------------------------

bool ts::FileNameRate::operator==(const FileNameRate& other) const
{
    return file_name == other.file_name && file_date == other.file_date && repetition == other.repetition;
}

bool ts::FileNameRate::operator<(const FileNameRate& other) const
{
    return file_name < other.file_name || file_date < other.file_date || repetition < other.repetition;
}


//----------------------------------------------------------------------------
// Scan the file for update.
//----------------------------------------------------------------------------

bool ts::FileNameRate::scanFile(size_t retry, Report& report)
{
    if (file_name.empty() || inline_xml) {
        // No file, no change...
        return false;
    }
    else {
        // Get new file time, will get Epoch if the file does not exist.
        const Time date = GetFileModificationTimeLocal(file_name);
        const bool changed = date != file_date;
        if (changed) {
            report.verbose(u"file %s %s", {file_name, file_date == Time::Epoch ? u"created" : (date == Time::Epoch ? u"deleted" : u"modified")});
            file_date = date;
            retry_count = retry;
        }
        // Return true if file was changed or some retries are allowed.
        return changed || retry_count > 0;
    }
}
