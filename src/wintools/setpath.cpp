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
#include "tsFormat.h"
#include "tsSysUtils.h"
#include "tsStringUtils.h"


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    enum UpdateCommand {APPEND, PREPEND, REMOVE};
    std::string   directory;
    std::string   registryKey;
    std::string   registryValue;
    UpdateCommand command;
};

Options::Options(int argc, char *argv[]) :
    ts::Args("Add or remove a directory to the system Path.", "[options] directory"),
    directory(),
    registryKey("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
    registryValue("Path"),
    command(APPEND)
{
    option("",         0,  Args::STRING, 1, 1);
    option("append",  'a');
    option("prepend", 'p');
    option("remove",  'r');

    setHelp("Directory:\n"
            "\n"
            "  A directory to add or remove to the system Path.\n"
            "\n"
            "Options\n"
            "\n"
            "  -a\n"
            "  --append\n"
            "    Append the directory to the system path (the default).\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  -p\n"
            "  --prepend\n"
            "    Prepend the directory to the system path.\n"
            "\n"
            "  -r\n"
            "  --remove\n"
            "    Remove the directory from the system path.\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");

    analyze(argc, argv);

    directory = value("");
    if (present("append")) {
        command = APPEND;
    }
    if (present("prepend")) {
        command = PREPEND;
    }
    if (present("remove")) {
        command = REMOVE;
    }
}


//-----------------------------------------------------------------------------
// Cleanup a directory path.
//-----------------------------------------------------------------------------

namespace {
    std::string CleanupDirectory(const std::string& path)
    {
        std::string directory(ts::VernacularFilePath(path));
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

#if defined(__windows)

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
    opt.debug(ts::Format("Path size: %d bytes, type: %d", int(pathSize), int(type)));
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        opt.error("invalid data type in " + opt.registryKey + "\\" + opt.registryValue);
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
    std::string pathValue(reinterpret_cast<char*>(&pathBuffer[0]));
    opt.debug("Path value: " + pathValue);

    // Split the Path into a list of clean directories.
    ts::StringList dirs;
    ts::SplitString(dirs, pathValue, ts::SearchPathSeparator, true);
    for (ts::StringList::iterator it = dirs.begin(); it != dirs.end(); ++it) {
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
    pathValue = ts::JoinStrings(dirs, std::string(1, ts::SearchPathSeparator));
    opt.debug("new Path value: " + pathValue);

    // Update the Path in the registry.
    // Always set type as REG_EXPAND_SZ, in case there is a variable reference in the add path.
    // Make sure the trailing nul character is included in the data size.
    status = ::RegSetValueEx(hkey, opt.registryValue.c_str(), 0, REG_EXPAND_SZ, reinterpret_cast<const ::BYTE*>(pathValue.c_str()), ::DWORD(pathValue.size()) + 1);
    if (status != ERROR_SUCCESS) {
        opt.error(ts::ErrorCodeMessage(status));
    }

    // Notify all applications that the Path was updated.
    ::SendNotifyMessageW(HWND_BROADCAST, WM_WININICHANGE, 0, reinterpret_cast<LPARAM>(L"Environment"));

#else
    opt.error("no effect on non-Windows systems");
#endif

    return EXIT_SUCCESS;
}
