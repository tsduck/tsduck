//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsOutputPager.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"

// Default name of the environment variable containing the pager command.
const ts::UChar* const ts::OutputPager::DEFAULT_PAGER = u"PAGER";


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::OutputPager::OutputPager(const UString& envName, bool stdoutOnly)
{
    // Check if we have a terminal.
    const bool outTerm = StdOutIsTerminal();
    const bool errTerm = StdErrIsTerminal();
    _hasTerminal = outTerm || (!stdoutOnly && errTerm);

    // Check if we should redirect one output.
    if (outTerm && !errTerm) {
        _outputMode = STDOUT_ONLY;
    }
    else if (!outTerm && errTerm) {
        _outputMode = STDERR_ONLY;
    }

    // Get the pager command.
    // First, check if the PAGER variable contains something.
    if (!envName.empty()) {
        _pagerCommand = ts::GetEnvironment(envName);
        _pagerCommand.trim();
    }

    // If there is no pager command, try to find one.
    if (_pagerCommand.empty()) {

        // Get the path search list.
        UStringList dirs;
        GetEnvironmentPath(dirs, PATH_ENVIRONMENT_VARIABLE);

        // Predefined list of commands.
        struct PredefinedPager {
            UString command;
            UString parameters;
        };
        const std::list<PredefinedPager> pagers({
            {u"less", u"-QFX"},
            {u"more", u""}
        });

        // Search the predefined pager commands in the path.
        for (auto itPager = pagers.begin(); itPager != pagers.end() && _pagerCommand.empty(); ++itPager) {
            for (auto itDir = dirs.begin(); itDir != dirs.end() && _pagerCommand.empty(); ++itDir) {
                // Full path of executable file.
                const UString exe(*itDir + fs::path::preferred_separator + itPager->command + EXECUTABLE_FILE_SUFFIX);
                if (fs::exists(exe)) {
                    // The executable exists.
                    bool useParameters = true;
                    // On Linux, with the BusyBox environment, many commands are redirected to the busybox executable.
                    // In that case, the busybox version may not understand some options of the GNU version.
                    #if defined(TS_LINUX)
                        useParameters = !UString(fs::weakly_canonical(exe, &ErrCodeReport())).contain(u"busybox", CASE_INSENSITIVE);
                    #endif
                    // Same thing with UnxUtils (sometimes spelled UnixUtils) on Windows.
                    #if defined(TS_WINDOWS)
                        useParameters = !exe.contain(u"unxutils", CASE_INSENSITIVE) && !exe.contain(u"unixutils", CASE_INSENSITIVE);
                    #endif
                    // Build the command line.
                    _pagerCommand = u'"' + exe + u"\" " + (useParameters ? itPager->parameters : UString());
                }
            }
        }
    }

    // On Windows, we can always use the embedded "more" command.
#if defined(TS_WINDOWS)
    if (_pagerCommand.empty()) {
        _pagerCommand = u"cmd /d /q /c more";
    }
#endif
}

ts::OutputPager::~OutputPager()
{
}


//----------------------------------------------------------------------------
// Create the process, open the pipe.
//----------------------------------------------------------------------------

bool ts::OutputPager::open(bool synchronous, size_t buffer_size, Report& report)
{
    if (!_hasTerminal) {
        report.error(u"not a terminal, cannot page");
        return false;
    }
    else if (_pagerCommand.empty()) {
        report.error(u"no pager command found, cannot page");
        return false;
    }
    else {
        return ForkPipe::open(_pagerCommand, synchronous ? ForkPipe::SYNCHRONOUS : ForkPipe::ASYNCHRONOUS, buffer_size, report, _outputMode, STDIN_PIPE);
    }
}


//----------------------------------------------------------------------------
// Write data to the pipe (received at process' standard input).
//----------------------------------------------------------------------------

bool ts::OutputPager::write(const UString& text, Report& report)
{
    const std::string utf8Text(text.toUTF8());
    size_t outsize = 0;
    return ForkPipe::writeStream(utf8Text.data(), utf8Text.size(), outsize, report);
}
