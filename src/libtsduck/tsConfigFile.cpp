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
//  Configuration files management
//
//----------------------------------------------------------------------------

#include "tsConfigFile.h"
#include "tsStringUtils.h"
#include "tsSysUtils.h"



//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ConfigFile::ConfigFile(const std::string& filename, ReportInterface& report) :
    _filename(filename),
    _sections(),
    _empty()
{
    if (filename != "") {
        load(filename, report);
    }
}

ts::ConfigFile::ConfigFile(std::istream& strm) :
    _filename(),
    _sections(),
    _empty()
{
    merge(strm);
}


//----------------------------------------------------------------------------
// Default configuration file name (executable file name with ".ini" extension)
//----------------------------------------------------------------------------

std::string ts::ConfigFile::DefaultFileName ()
{
    return PathPrefix(ExecutableFile()) + ".ini";
}


//----------------------------------------------------------------------------
// Reset content of the configuration
//----------------------------------------------------------------------------

void ts::ConfigFile::reset()
{
    _sections.clear();
}


//----------------------------------------------------------------------------
// Reload configuration from a file
//----------------------------------------------------------------------------

bool ts::ConfigFile::load(const std::string& filename, ReportInterface& report)
{
    reset();
    return merge(filename, report);
}


//----------------------------------------------------------------------------
// Merge configuration from a file.
//----------------------------------------------------------------------------

bool ts::ConfigFile::merge(const std::string& filename, ReportInterface& report)
{
    // Save file name for further save
    _filename = filename;

    // Open the file
    std::ifstream file(_filename.c_str());

    // Non-existent file means empty configuration.
    // Report a warning.
    if (!file) {
        report.error("Cannot open configuration file " + _filename);
        return false;
    }

    // Parse the content.
    merge(file);
    return true;
}

void ts::ConfigFile::merge(std::istream& strm)
{
    // Initial section is ""
    std::string section;
    char line[2048];

    while (strm.getline(line, sizeof(line))) {
        char *start, *value;

        // Remove comment
        if ((value = ::strchr(line, '#')) != 0) {
            *value = 0;
        }

        // Remove leading blanks
        for (start = line; ::isspace(*start); ++start) {}

        if (*start == '[') {
            // Handle section name
            char* end = ::strchr(start, ']');
            if (end != 0) {
                *end = 0;
            }
            section = start + 1;
            Trim(section);
            // Implicitely creates the section
            _sections[section];
        }
        else if (::strchr(start, '=') != 0) {
            // Handle entry definition
            _sections[section].set(start);
        }
    }
}


//----------------------------------------------------------------------------
// Save a configuration file.
// If no file name is specified, use name from constructor or load()
//----------------------------------------------------------------------------

bool ts::ConfigFile::save (const std::string& filename, ReportInterface& report) const
{
    // Get file name
    if (filename != "") {
        _filename = filename;
    }
    if (_filename == "") {
        report.error ("no file name specified to save configuration");
        return false;
    }

    // Create the file
    std::ofstream file (_filename.c_str());

    if (!file) {
        report.error ("error creating configuration file");
        return false;
    }

    // Save the content
    return save(file).good();
}


//----------------------------------------------------------------------------
// Save a configuration file in a stream
//----------------------------------------------------------------------------

std::ostream& ts::ConfigFile::save (std::ostream& strm) const
{
    // First, save content of section "" (out of any section)
    SectionMap::const_iterator sec (_sections.find (""));
    if (sec != _sections.end ()) {
        sec->second.save (strm);
    }

    // Then, save all sections, skipping section ""
    for (sec = _sections.begin(); strm && sec != _sections.end(); ++sec) {
        if (sec->first != "") {
            strm << std::endl << "[" << sec->first << "]" << std::endl;
            sec->second.save (strm);
        }
    }

    return strm;
}

//----------------------------------------------------------------------------
// Get the number of sections in the file.
//----------------------------------------------------------------------------

size_t ts::ConfigFile::sectionCount () const
{
    return _sections.size ();
}

//----------------------------------------------------------------------------
// Get the names of all sections
//----------------------------------------------------------------------------

void ts::ConfigFile::getSectionNames(StringVector& names) const
{
    names.clear();
    for (SectionMap::const_iterator sec = _sections.begin(); sec != _sections.end(); ++sec) {
        names.push_back(sec->first);
    }
}

//----------------------------------------------------------------------------
// Get a reference to a section.
// Return a reference to an empty section if does not exist.
//----------------------------------------------------------------------------

const ts::ConfigSection& ts::ConfigFile::section(const std::string& name) const
{
    SectionMap::const_iterator sec = _sections.find(name);
    return sec == _sections.end() ? _empty : sec->second;
}
