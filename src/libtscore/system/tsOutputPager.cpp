//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsOutputPager.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"


//----------------------------------------------------------------------------
// Constructors and destructor.
//----------------------------------------------------------------------------

ts::OutputPager::OutputPager(Report* report, const UString& env_name, bool stdout_only, Object* owner) :
    ForkPipeOutputStream(report, owner)
{
    init(env_name, stdout_only);
}

ts::OutputPager::OutputPager(ReporterBase* delegate, const UString& env_name, bool stdout_only, Object* owner) :
    ForkPipeOutputStream(delegate, owner)
{
    init(env_name, stdout_only);
}

ts::OutputPager::~OutputPager()
{
}


//----------------------------------------------------------------------------
// Constructors common code.
//----------------------------------------------------------------------------

void ts::OutputPager::init(const UString& env_name, bool stdout_only)
{
    // Check if we have a terminal.
    const bool out_term = StdOutIsTerminal();
    const bool err_term = StdErrIsTerminal();
    _has_terminal = out_term || (!stdout_only && err_term);

    // Check if we should redirect one output.
    if (out_term && !err_term) {
        _output_mode = STDOUT_ONLY;
    }
    else if (!out_term && err_term) {
        _output_mode = STDERR_ONLY;
    }

    // Get the pager command.
    // First, check if the PAGER variable contains something.
    if (!env_name.empty()) {
        _pager_command = ts::GetEnvironment(env_name);
        _pager_command.trim();
    }

    // If there is no pager command, try to find one.
    if (_pager_command.empty()) {

        // Get the path search list.
        UStringList dirs;
        GetEnvironmentPath(dirs, PATH_ENVIRONMENT_VARIABLE);

        // Predefined list of commands.
        struct PredefinedPager {
            UString command;
            UString parameters;
        };
        const std::list<PredefinedPager> pagers({
            {u"less", u"-QFXS"},
            {u"more", u""}
        });

        // Search the predefined pager commands in the path.
        for (auto it_pager = pagers.begin(); it_pager != pagers.end() && _pager_command.empty(); ++it_pager) {
            for (auto it_dir = dirs.begin(); it_dir != dirs.end() && _pager_command.empty(); ++it_dir) {
                // Full path of executable file.
                const UString exe(*it_dir + fs::path::preferred_separator + it_pager->command + EXECUTABLE_FILE_SUFFIX);
                if (fs::exists(exe)) {
                    // The executable exists.
                    bool use_parameters = true;
                    // On Linux, with the BusyBox environment, many commands are redirected to the busybox executable.
                    // In that case, the busybox version may not understand some options of the GNU version.
                    #if defined(TS_LINUX)
                        use_parameters = !UString(fs::weakly_canonical(exe, &ErrCodeReport())).contains(u"busybox", CASE_INSENSITIVE);
                    #endif
                    // Same thing with UnxUtils (sometimes spelled UnixUtils) on Windows.
                    #if defined(TS_WINDOWS)
                        use_parameters = !exe.contains(u"unxutils", CASE_INSENSITIVE) && !exe.contains(u"unixutils", CASE_INSENSITIVE);
                    #endif
                    // Build the command line.
                    _pager_command = u'"' + exe + u"\" " + (use_parameters ? it_pager->parameters : UString());
                }
            }
        }
    }

    // On Windows, we can always use the embedded "more" command.
#if defined(TS_WINDOWS)
    if (_pager_command.empty()) {
        _pager_command = u"cmd /d /q /c more";
    }
#endif
}


//----------------------------------------------------------------------------
// Create the process, open the pipe.
//----------------------------------------------------------------------------

bool ts::OutputPager::open(bool synchronous, size_t buffer_size)
{
    if (!_has_terminal) {
        report().error(u"not a terminal, cannot page");
        return false;
    }
    else if (_pager_command.empty()) {
        report().error(u"no pager command found, cannot page");
        return false;
    }
    else {
        return ForkPipeOutputStream::open(_pager_command, synchronous ? ForkPipe::SYNCHRONOUS : ForkPipe::ASYNCHRONOUS, buffer_size, _output_mode, STDIN_PIPE);
    }
}


//----------------------------------------------------------------------------
// Write data to the pipe (received at process' standard input).
//----------------------------------------------------------------------------

bool ts::OutputPager::write(const UString& text)
{
    const std::string utf8Text(text.toUTF8());
    size_t outsize = 0;
    return ForkPipeOutputStream::writeStream(utf8Text.data(), utf8Text.size(), outsize);
}
