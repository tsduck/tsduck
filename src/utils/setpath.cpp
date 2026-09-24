//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
// This program is used to manipulate the system Path on Windows. it is
// useful to add / remove the path to an application in an installer.
// NSIS has the ability to do the same thing using the extension
// "EnvVarUpdate". However, there is a limitation in NSIS; all strings are
// limited to 1024 characters. This means that if the Path already contains
// or will contain more than 1024 characters after the update, the Path is
// simply emptied. This program is made to overcome this limitation. It is
// typically installed with the application and executed during installation
// and deinstallation.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsErrCodeReport.h"
#include "tsByteBlock.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsWinUtils.h"
#include "tsRegistry.h"


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class Options: public ts::Args
{
    TS_NOBUILD_NOCOPY(Options);
public:
    Options(int argc, char *argv[]);
    enum UpdateCommand {APPEND, PREPEND, REMOVE, STATUS};
    ts::UString   directory {};
    ts::UString   environment {};
    ts::UString   hard_link {};
    ts::UString   sym_link {};
    UpdateCommand command = APPEND;
    bool          initial_separator = false;
    bool          final_separator = false;
    bool          dry_run = false;
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Add or remove a directory to the system Path.", u"[options] directory")
{
    const ts::UString sep(1, ts::SEARCH_PATH_SEPARATOR);

    option(u"", 0, Args::FILENAME, 1, 1);
    help(u"", u"A directory to add or remove to the system Path.");

    option(u"append", 'a');
    help(u"append", u"Append the directory to the system path (this is the default).");

    option(u"dry-run", 'n');
    help(u"dry-run", u"Display what would be done, but does not do anything.");

    option(u"environment", 'e', Args::STRING);
    help(u"environment", u"Name of the path environment variable. The default is \"Path\".");

    option(u"final-separator", 'f');
    help(u"final-separator", u"Force a final '" + sep + u"' at the end of the system path.");

    option(u"hard-link", 'h', Args::FILENAME);
    help(u"hard-link", u"Don't update any path. Create the specified hard link pointing to the command parameter.");

    option(u"initial-separator", 'i');
    help(u"initial-separator", u"Force an initial '" + sep + u"' at the beginning of the system path.");

    option(u"link", 'l', Args::FILENAME);
    help(u"link", u"Don't update any path. Create the specified symbolic link pointing to the command parameter.");

    option(u"prepend", 'p');
    help(u"prepend", u"Prepend the directory to the system path.");

    option(u"remove", 'r');
    help(u"remove", u"Remove the directory from the system path.");

    option(u"status", 's');
    help(u"status", u"Don't update any path. Use the parameter as an integer value and display the corresponding error code.");

    analyze(argc, argv);

    getValue(directory, u"");
    getValue(environment, u"environment", u"Path");
    getValue(hard_link, u"hard-link");
    getValue(sym_link, u"link");
    initial_separator = present(u"initial-separator");
    final_separator = present(u"final-separator");
    dry_run = present(u"dry-run");

    if (present(u"append")) {
        command = APPEND;
    }
    if (present(u"prepend")) {
        command = PREPEND;
    }
    if (present(u"remove")) {
        command = REMOVE;
    }
    if (present(u"status")) {
        command = STATUS;
    }
}


//-----------------------------------------------------------------------------
// Cleanup a directory path.
//-----------------------------------------------------------------------------

namespace {
    ts::UString CleanupDirectory(const ts::UString& path)
    {
        ts::UString directory(ts::VernacularFilePath(path));
        while (!directory.empty() && directory[directory.size() - 1] == fs::path::preferred_separator) {
            directory.resize(directory.size() - 1);
        }
        return directory;
    }
}


//-----------------------------------------------------------------------------
// Program entry point
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Decode command line.
    Options opt(argc, argv);

    // Specific case of displaying a Windows error code.
    if (opt.command == Options::STATUS) {
        std::intmax_t status = 0;
        if (!opt.directory.toInteger(status, ts::UString::DEFAULT_THOUSANDS_SEPARATOR)) {
            opt.fatal(u"invalid integer status value: %s", opt.directory);
        }
        ::DWORD status32 = ::DWORD(status & 0xFFFFFFFF);
        if (opt.verbose()) {
            std::cout << ts::UString::Format(u"0x%08X => ", status32);
        }
        std::cout << ts::WinErrorMessage(status32) << std::endl;
        return EXIT_SUCCESS;
    }

    // Specific case of creating symbolic or hard link.
    if (!opt.sym_link.empty()) {
        fs::create_symlink(opt.directory, opt.sym_link, &ts::ErrCodeReport(opt, u"error creating symbolic link", opt.sym_link));
        return EXIT_SUCCESS;
    }
    if (!opt.hard_link.empty()) {
        fs::create_hard_link(opt.directory, opt.hard_link, &ts::ErrCodeReport(opt, u"error creating hard link", opt.hard_link));
        return EXIT_SUCCESS;
    }

    // Get the Path value.
    ts::UString path(ts::Registry::GetValue(ts::Registry::SystemEnvironmentKey, opt.environment, opt));
    if (path.empty() && opt.environment.similar(u"Path")) {
        opt.fatal(u"cannot get path from registry: %s\\%s", ts::Registry::SystemEnvironmentKey, opt.environment);
    }
    if (opt.dry_run) {
        opt.info(u"Previous %s value: %s", opt.environment, path);
    }

    // Split the Path into a list of clean directories.
    ts::UStringList dirs;
    path.split(dirs, ts::SEARCH_PATH_SEPARATOR, true, true);
    for (auto& it : dirs) {
        it = CleanupDirectory(it);
    }

    // Remove the specified directory from the Path, if already present.
    dirs.remove(opt.directory);

    // Add directory if required.
    switch (opt.command) {
        case Options::APPEND:
            dirs.push_back(opt.directory);
            break;
        case Options::PREPEND:
            dirs.push_front(opt.directory);
            break;
        case Options::REMOVE:
        case Options::STATUS:
        default:
            // Nothing to do
            break;
    }

    // Rebuild the new Path.
    path = ts::UString::Join(dirs, ts::UString(1, ts::SEARCH_PATH_SEPARATOR));
    if (opt.initial_separator) {
        path.insert(path.begin(), ts::SEARCH_PATH_SEPARATOR);
    }
    if (opt.final_separator) {
        path.append(ts::SEARCH_PATH_SEPARATOR);
    }
    if (opt.dry_run) {
        opt.info(u"New %s value: %s", opt.environment, path);
    }
    else {
        // Update the Path in the registry.
        // Always set type as REG_EXPAND_SZ, in case there is a variable reference in the add path.
        if (!ts::Registry::SetValue(ts::Registry::SystemEnvironmentKey, opt.environment, path, true, opt)) {
            opt.fatal(u"error setting path in registry: %s\\%s", ts::Registry::SystemEnvironmentKey, opt.environment);
        }

        // Notify all applications that the Path was updated.
        ts::Registry::NotifyEnvironmentChange(opt);
    }

    return EXIT_SUCCESS;
}
