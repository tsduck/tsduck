//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Decode file names / repetition rates command line arguments
//
//----------------------------------------------------------------------------

#include "tsFileNameRate.h"
#include "tsToInteger.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Default constructor.
//----------------------------------------------------------------------------

ts::FileNameRate::FileNameRate(const std::string& name, MilliSecond rep) :
    file_name(name),
    file_date(),
    repetition(rep) ,
    retry_count(0)
{
}


//----------------------------------------------------------------------------
// Comparison operators.
//----------------------------------------------------------------------------

bool ts::FileNameRate::operator==(const FileNameRate& other)
{
    return file_name == other.file_name && file_date == other.file_date && repetition == other.repetition;
}

bool ts::FileNameRate::operator<(const FileNameRate& other)
{
    return file_name < other.file_name || file_date < other.file_date || repetition < other.repetition;
}


//----------------------------------------------------------------------------
// Scan the file for update.
//----------------------------------------------------------------------------

bool ts::FileNameRate::scanFile(size_t retry, ReportInterface& report)
{
    if (file_name.empty()) {
        // No file, no change...
        return false;
    }
    else {
        // Get new file time, will get Epoch if the file does not exist.
        const Time date = GetFileModificationTimeLocal(file_name);
        const bool changed = date != file_date;
        if (changed) {
            file_date = date;
            retry_count = retry;
            report.verbose("file %s %s", file_name.c_str(), file_date == Time::Epoch ? "created" : (date == Time::Epoch ? "deleted" : "modified"));
        }
        // Return true if file was changed or some retries are allowed.
        return changed || retry_count > 0;
    }
}


//----------------------------------------------------------------------------
// Scan the files for update.
//----------------------------------------------------------------------------

size_t ts::FileNameRateList::scanFiles(size_t retry, ReportInterface& report)
{
    size_t count = 0;
    for (iterator it = begin(); it != end(); ++it) {
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

bool ts::FileNameRateList::getArgs(Args& args, const char* option_name, MilliSecond default_rate)
{
    // Get the string values
    StringVector strings;
    args.getValues(strings, option_name);

    // Decode the args
    clear();
    bool success = true;

    for (size_t i = 0; i < strings.size(); ++i) {
        const std::string::size_type eq = strings[i].find('=');
        FileNameRate file;
        if (eq == std::string::npos) {
            // No '=' found
            file.file_name = strings[i];
            file.repetition = default_rate;
        }
        else {
            file.file_name = strings[i].substr(0, eq);
            if (!ToInteger(file.repetition, strings[i].substr(eq + 1)) || file.repetition <= 0) {
                args.error("invalid repetition rate for file " + file.file_name);
                file.repetition = default_rate;
                success = false;
            }
        }
        push_back(file);
    }

    return success;
}
