//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2017, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  This program is used to manipulate the system Path on Windows. it is
//  useful to add / remove the path to an application in an installer.
//  NSIS has the ability to do the same thing using the extension
//  "EnvVarUpdate". However, there is a limitation in NSIS; all strings are
//  limited to 1024 characters. This means that if the Path already contains
//  or will contain more than 1024 characters after the update, the Path is
//  simply emptied. This program is made to overcome this limitation. It is
//  typically installed with the application and executed during installation
//  and deinstallation.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsByteBlock.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    enum UpdateCommand {APPEND, PREPEND, REMOVE};
    ts::UString   directory;
    std::string   registryKey;
    std::string   registryValue;
    UpdateCommand command;
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Add or remove a directory to the system Path.", u"[options] directory"),
    directory(),
    registryKey("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
    registryValue("Path"),
    command(APPEND)
{
    option(u"",         0,  Args::STRING, 1, 1);
    option(u"append",  'a');
    option(u"prepend", 'p');
    option(u"remove",  'r');

    setHelp(u"Directory:\n"
            u"\n"
            u"  A directory to add or remove to the system Path.\n"
            u"\n"
            u"Options\n"
            u"\n"
            u"  -a\n"
            u"  --append\n"
            u"    Append the directory to the system path (the default).\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -p\n"
            u"  --prepend\n"
            u"    Prepend the directory to the system path.\n"
            u"\n"
            u"  -r\n"
            u"  --remove\n"
            u"    Remove the directory from the system path.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    directory = value(u"");
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

#if defined(TS_WINDOWS)

    // Access registry value.
    ::HKEY hkey;
    ::LONG status = ::RegOpenKey(HKEY_LOCAL_MACHINE, opt.registryKey.c_str(), &hkey);
    if (status != ERROR_SUCCESS) {
        opt.fatal(ts::ErrorCodeMessage(status));
    }

    // Get the size and type of the Path value.
    ::DWORD type = 0;
    ::DWORD pathSize = 0;
    status = ::RegQueryValueEx(hkey, opt.registryValue.c_str(), NULL, &type, NULL, &pathSize);
    if (status != ERROR_SUCCESS) {
        opt.error(ts::ErrorCodeMessage(status));
    }
    opt.debug(u"Path size: %d bytes, type: %d", {pathSize, type});
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        opt.error(u"invalid data type in %s\\%s", {opt.registryKey, opt.registryValue});
    }

    // Get the Path value.
    ts::ByteBlock pathBuffer(pathSize + 256);
    pathSize = ::DWORD(pathBuffer.size());
    status = ::RegQueryValueEx(hkey, opt.registryValue.c_str(), NULL, &type, &pathBuffer[0], &pathSize);
    if (status != ERROR_SUCCESS) {
        opt.error(ts::ErrorCodeMessage(status));
    }
    pathBuffer.resize(std::min(pathBuffer.size(), size_t(pathSize)));

    // Turn the path value into a string.
    pathBuffer.push_back(0);
    ts::UString pathValue(reinterpret_cast<char*>(&pathBuffer[0]));
    opt.debug(u"Path value: %s", {pathValue});

    // Split the Path into a list of clean directories.
    ts::UStringList dirs;
    pathValue.split(dirs, ts::SearchPathSeparator, true);
    for (ts::UStringList::iterator it = dirs.begin(); it != dirs.end(); ++it) {
        *it = CleanupDirectory(*it);
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
        default:
            // Nothing to do
            break;
    }

    // Rebuild the new Path.
    pathValue = ts::UString::Join(dirs, std::string(1, ts::SearchPathSeparator));
    opt.debug(u"new Path value: %s", {pathValue});

    // Update the Path in the registry.
    // Always set type as REG_EXPAND_SZ, in case there is a variable reference in the add path.
    // Make sure the trailing nul character is included in the data size.
    const std::string pathValueUTF8(pathValue.toUTF8());
    status = ::RegSetValueEx(hkey, opt.registryValue.c_str(), 0, REG_EXPAND_SZ, reinterpret_cast<const ::BYTE*>(pathValueUTF8.c_str()), ::DWORD(pathValue.size()) + 1);
    if (status != ERROR_SUCCESS) {
        opt.error(ts::ErrorCodeMessage(status));
    }

    // Notify all applications that the Path was updated.
    ::SendNotifyMessageW(HWND_BROADCAST, WM_WININICHANGE, 0, reinterpret_cast<LPARAM>(L"Environment"));

#else
    opt.error(u"no effect on non-Windows systems");
#endif

    return EXIT_SUCCESS;
}
