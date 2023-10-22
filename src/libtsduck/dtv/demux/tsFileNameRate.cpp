//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    repetition(rep)
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
