//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//  Checking TSDuck versions, download and upgrade new versions.
//  Information about new releases are fetched from GitHub using its Web API.
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsGitHubRelease.h"
#include "tsVersionInfo.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public ts::Args
{
    Options(int argc, char *argv[]);

    bool        current;   // Display current version of TSDuck, this executable.
    bool        latest;    // Display the latest version of TSDuck.
    bool        check;     // Check if a new version of TSDuck is available.
    bool        all;       // List all available versions of TSDuck.
    bool        download;  // Download the lastest version.
    bool        binary;    // With --download, fetch the binaries.
    bool        source;    // With --download, feth the source code instead of the binaries.
    bool        upgrade;   // Upgrade TSDuck to the latest version.
    ts::UString name;      // Use the specified version, not the latest one.
    ts::UString out_dir;   // Output directory for downloaded files.

private:
    // Inaccessible operations.
    Options(const Options&) = delete;
    Options& operator=(const Options&) = delete;
};

Options::Options(int argc, char *argv[]) :
    ts::Args(u"Check version, download and upgrade TSDuck.", u"[options]"),
    current(false),
    latest(false),
    check(false),
    all(false),
    download(false),
    binary(false),
    source(false),
    upgrade(false),
    name(),
    out_dir()
{
    option(u"all",              'a');
    option(u"binary",           'b');
    option(u"check",            'c');
    option(u"download",         'd');
    option(u"latest",           'l');
    option(u"name",             'n', Args::STRING);
    option(u"output-directory", 'o', Args::STRING);
    option(u"source",           's');
    option(u"this",             't');
    option(u"upgrade",          'u');

    setHelp(u"Options:\n"
            u"\n"
            u"  -a\n"
            u"  --all\n"
            u"      List all available versions of TSDuck from GitHub.\n"
            u"\n"
            u"  -b\n"
            u"  --binary\n"
            u"      With --download, fetch the binary installers of the latest version. This\n"
            u"      is the default. When --source is specified, specify --binary if you also\n"
            u"      need the binary installers.\n"
            u"\n"
            u"  -c\n"
            u"  --check\n"
            u"      Check if a new version of TSDuck is available from GitHub.\n"
            u"\n"
            u"  -d\n"
            u"  --download\n"
            u"      Download the lastest version (or the version specified by --name) from\n"
            u"      GitHub. By default, download the binary installers for the current\n"
            u"      operating system and architecture. Specify --source to download the\n"
            u"      source code.\n"
            u"\n"
            u"  --help\n"
            u"      Display this help text.\n"
            u"\n"
            u"  -l\n"
            u"  --latest\n"
            u"      Display the latest version of TSDuck from GitHub.\n"
            u"\n"
            u"  -n version-name\n"
            u"  --name version-name\n"
            u"      Get information for or download from GitHub the specified version, not\n"
            u"      the latest one.\n"
            u"\n"
            u"  -o dir-name\n"
            u"  --output-directory dir-name\n"
            u"      Output directory for downloaded files (current directory by default).\n"
            u"\n"
            u"  -s\n"
            u"  --source\n"
            u"      With --download, download the source code archive instead of the binary\n"
            u"      installers.\n"
            u"\n"
            u"  -t\n"
            u"  --this\n"
            u"      Display the current version of TSDuck (this executable).\n"
            u"\n"
            u"  -u\n"
            u"  --upgrade\n"
            u"      Upgrade TSDuck to the latest version.\n"
            u"\n"
            u"  -v\n"
            u"  --verbose\n"
            u"      Produce verbose output.\n"
            u"\n"
            u"  --version\n"
            u"      Display the version number.\n");

    analyze(argc, argv);

    all = present(u"all");
    current = present(u"this");
    latest = present(u"latest");
    check = present(u"check");
    download = present(u"download");
    binary = present(u"binary");
    source = present(u"source");
    upgrade = present(u"upgrade");
    getValue(name, u"name");
    getValue(out_dir, u"output-directory");

    exitOnError();
}


//----------------------------------------------------------------------------
//  List all versions.
//----------------------------------------------------------------------------

bool ListAllVersions(Options& opt)
{
    // Get all releases.
    ts::GitHubReleaseVector rels;
    if (!ts::GitHubRelease::GetAllVersions(rels, u"tsduck", u"tsduck", opt)) {
        return false;
    }

    // List them all.
    for (ts::GitHubReleaseVector::const_iterator it = rels.begin(); it != rels.end(); ++it) {
        if (!it->isNull()) {
            const ts::GitHubRelease& rel(**it);
            if (opt.verbose()) {

            }
            else {
                std::cout << rel.version() << std::endl;
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
//  Process one version.
//----------------------------------------------------------------------------

bool ProcessVersion(Options& opt)
{
    //@@@@@
    return true;
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    TSDuckLibCheckVersion();
    Options opt(argc, argv);
    bool success = true;

    if (opt.current) {
        // Display current version.
        std::cout << ts::GetVersion(opt.verbose() ? ts::VERSION_LONG : ts::VERSION_SHORT) << std::endl;
    }
    else if (opt.all) {
        success = ListAllVersions(opt);
    }
    else {
        success = ProcessVersion(opt);
    }

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
