//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsConfigFile.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"

// Used to return a constant reference.
const ts::ConfigSection ts::ConfigFile::_empty;


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::ConfigFile::ConfigFile(const UString& filename, Report& report, const UString env_disable) :
    ConfigFile(filename, UString(), report, env_disable)
{
}

ts::ConfigFile::ConfigFile(const UString& filename1, const UString& filename2, Report& report, const UString env_disable)
{
    // Only if not disabled by environment variable.
    if (env_disable.empty() || GetEnvironment(env_disable).empty()) {

        // Try to load first file.
        if (!filename1.empty()) {
            load(filename1, report);
        }
        // If nothing was loaded from first file, try second file.
        if (!filename2.empty() && _sections.empty()) {
            load(filename2, report);
        }
    }
}

ts::ConfigFile::ConfigFile(std::istream& strm)
{
    merge(strm);
}


//----------------------------------------------------------------------------
// Default configuration file name (executable file name with ".ini" extension)
//----------------------------------------------------------------------------

ts::UString ts::ConfigFile::DefaultFileName(FileStyle style, const UString& name)
{
    if (style == LOCAL_SYSTEM) {
#if defined(TS_WINDOWS)
        style = WINDOWS_STYLE;
#else
        style = UNIX_STYLE;
#endif
    }

    UString fileName(name);
    if (fileName.empty()) {
        fileName = PathPrefix(BaseName(ExecutableFile()));
    }

    if (style == WINDOWS_STYLE) {
        return DirectoryName(ExecutableFile()) + fs::path::preferred_separator + fileName + u".ini";
    }
    else {
        return (UserHomeDirectory() + fs::path::preferred_separator) + u'.' + fileName;
    }
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

bool ts::ConfigFile::load(const UString& filename, Report& report)
{
    reset();
    return merge(filename, report);
}


//----------------------------------------------------------------------------
// Merge configuration from a file.
//----------------------------------------------------------------------------

bool ts::ConfigFile::merge(const UString& filename, Report& report)
{
    // Save file name for further save
    _filename = filename;

    // Open the file
    std::ifstream file(_filename.toUTF8().c_str());

    // Non-existent file means empty configuration.
    // Report a warning.
    if (!file) {
        report.error(u"Cannot open configuration file %s", {_filename});
        return false;
    }

    // Parse the content.
    merge(file);
    return true;
}

void ts::ConfigFile::merge(std::istream& strm)
{
    // Initial section is ""
    UString section;
    UString line;
    UString cont;
    size_t pos = 0;

    // Loop on all lines.
    while (line.getLine(strm)) {

        // Rebuild multi-line.
        while (line.endWith(u"\\")) {
            line.erase(line.size() - 1);
            if (!cont.getLine(strm)) {
                break;
            }
            line.append(cont);
        }

        // Remove leading blanks.
        line.trim(true, false);

        if (line.startWith(u"#")) {
            // Ignore comment lines.
        }
        else if (line.startWith(u"[")) {
            // Handle section name
            line.erase(0, 1);
            if ((pos = line.find(u']')) != NPOS) {
                line.erase(pos);
            }
            line.trim();
            section = line;
            // Implicitely creates the section
            _sections[section];
        }
        else if ((pos = line.find(u'=')) != NPOS) {
            // Handle entry definition
            UString name(line, 0, pos);
            UString val(line, pos + 1, NPOS);
            name.trim();
            val.trim();
            _sections[section].append(name, val);
        }
    }
}


//----------------------------------------------------------------------------
// Save a configuration file.
// If no file name is specified, use name from constructor or load()
//----------------------------------------------------------------------------

bool ts::ConfigFile::save(const UString& filename, Report& report) const
{
    // Get file name
    if (!filename.empty()) {
        _filename = filename;
    }
    if (_filename.empty()) {
        report.error(u"no file name specified to save configuration");
        return false;
    }

    // Create the file
    std::ofstream file(_filename.toUTF8().c_str());

    if (!file) {
        report.error(u"error creating configuration file %s", {_filename});
        return false;
    }

    // Save the content
    return save(file).good();
}


//----------------------------------------------------------------------------
// Save a configuration file in a stream
//----------------------------------------------------------------------------

std::ostream& ts::ConfigFile::save(std::ostream& strm) const
{
    // First, save content of section "" (out of any section)
    auto sec = _sections.find(UString());
    if (sec != _sections.end()) {
        sec->second.save(strm);
    }

    // Then, save all sections, skipping section ""
    for (sec = _sections.begin(); strm && sec != _sections.end(); ++sec) {
        if (!sec->first.empty()) {
            strm << std::endl << "[" << sec->first << "]" << std::endl;
            sec->second.save(strm);
        }
    }

    return strm;
}


//----------------------------------------------------------------------------
// Get the names of all sections
//----------------------------------------------------------------------------

void ts::ConfigFile::getSectionNames(UStringVector& names) const
{
    names.clear();
    for (const auto& sec : _sections) {
        names.push_back(sec.first);
    }
}


//----------------------------------------------------------------------------
// Get a reference to a section.
// Return a reference to an empty section if does not exist.
//----------------------------------------------------------------------------

const ts::ConfigSection& ts::ConfigFile::section(const UString& name) const
{
    const auto sec = _sections.find(name);
    return sec == _sections.end() ? _empty : sec->second;
}
