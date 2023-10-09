//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
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
#include "tsByteBlock.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsRegistry.h"


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

class Options: public ts::Args
{
    TS_NOBUILD_NOCOPY(Options);
public:
    Options(int argc, char *argv[]);
    enum UpdateCommand {APPEND, PREPEND, REMOVE};
    ts::UString   directory;
    ts::UString   environment;
    UpdateCommand command;
    bool          initialSeparator;
    bool          finalSeparator;
    bool          dryRun;
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Add or remove a directory to the system Path.", u"[options] directory"),
    directory(),
    environment(),
    command(APPEND),
    initialSeparator(false),
    finalSeparator(false),
    dryRun(false)
{
    const ts::UString sep(1, ts::SearchPathSeparator);

    option(u"", 0, Args::STRING, 1, 1);
    help(u"", u"A directory to add or remove to the system Path.");

    option(u"append", 'a');
    help(u"append", u"Append the directory to the system path (this is the default).");

    option(u"dry-run", 'n');
    help(u"dry-run", u"Display what would be done, but does not do anything.");

    option(u"environment", 'e', Args::STRING);
    help(u"environment", u"Name of the path environment variable. The default is \"Path\".");

    option(u"final-separator", 'f');
    help(u"final-separator", u"Force a final '" + sep + u"' at the end of the system path.");

    option(u"initial-separator", 'i');
    help(u"initial-separator", u"Force an initial '" + sep + u"' at the beginning of the system path.");

    option(u"prepend", 'p');
    help(u"prepend", u"Prepend the directory to the system path.");

    option(u"remove", 'r');
    help(u"remove", u"Remove the directory from the system path.");

    analyze(argc, argv);

    directory = value(u"");
    initialSeparator = present(u"initial-separator");
    finalSeparator = present(u"final-separator");
    dryRun = present(u"dry-run");
    getValue(environment, u"environment", u"Path");

    if (present(u"append")) {
        command = APPEND;
    }
    if (present(u"prepend")) {
        command = PREPEND;
    }
    if (present(u"remove")) {
        command = REMOVE;
    }
}


//-----------------------------------------------------------------------------
// Cleanup a directory path.
//-----------------------------------------------------------------------------

namespace {
    ts::UString CleanupDirectory(const ts::UString& path)
    {
        ts::UString directory(ts::VernacularFilePath(path));
        while (!directory.empty() && directory[directory.size() - 1] == ts::PathSeparator) {
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

    // Get the Path value.
    ts::UString path(ts::Registry::GetValue(ts::Registry::SystemEnvironmentKey, opt.environment, opt));
    if (path.empty() && opt.environment.similar(u"Path")) {
        opt.fatal(u"cannot get path from registry: %s\\%s", {ts::Registry::SystemEnvironmentKey, opt.environment});
    }
    if (opt.dryRun) {
        opt.info(u"Previous %s value: %s", {opt.environment, path});
    }

    // Split the Path into a list of clean directories.
    ts::UStringList dirs;
    path.split(dirs, ts::SearchPathSeparator, true, true);
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
        default:
            // Nothing to do
            break;
    }

    // Rebuild the new Path.
    path = ts::UString::Join(dirs, ts::UString(1, ts::SearchPathSeparator));
    if (opt.initialSeparator) {
        path.insert(path.begin(), ts::SearchPathSeparator);
    }
    if (opt.finalSeparator) {
        path.append(ts::SearchPathSeparator);
    }
    if (opt.dryRun) {
        opt.info(u"New %s value: %s", {opt.environment, path});
    }
    else {
        // Update the Path in the registry.
        // Always set type as REG_EXPAND_SZ, in case there is a variable reference in the add path.
        if (!ts::Registry::SetValue(ts::Registry::SystemEnvironmentKey, opt.environment, path, true, opt)) {
            opt.fatal(u"error setting path in registry: %s\\%s", {ts::Registry::SystemEnvironmentKey, opt.environment});
        }

        // Notify all applications that the Path was updated.
        ts::Registry::NotifyEnvironmentChange(opt);
    }

    return EXIT_SUCCESS;
}
