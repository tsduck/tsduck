//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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

#include "tsMain.h"
#include "tsVersionInfo.h"
#include "tsGitHubRelease.h"
#include "tsWebRequest.h"
#include "tsFileUtils.h"
#include "tsSysUtils.h"
#include "tsSysInfo.h"
#include "tsForkPipe.h"
#include "tsDuckExtensionRepository.h"
#if defined(TS_WINDOWS)
#include "tsWinUtils.h"
#endif
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace {
    class Options: public ts::Args
    {
        TS_NOBUILD_NOCOPY(Options);
    public:
        Options(int argc, char *argv[]);

        // The following options apply to the current instance of TSDuck.
        // They are always available.

        bool        current;    // Display current version of TSDuck, this executable.
        bool        integer;    // Display current version of TSDuck as integer value.
        bool        extensions; // List extensions.

        // The following options are used to detect, download and upgrade new versions of TSDuck.
        // They are disabled when TS_NO_GITHUB is defined. With this macro, TSDuck is unlinked
        // from the generic binary distribution on GitHub. Thus, upgrades can be limited to the
        // repositories of a given Linux distro.

#if !defined(TS_NO_GITHUB)
        bool        latest;     // Display the latest version of TSDuck.
        bool        check;      // Check if a new version of TSDuck is available.
        bool        all;        // List all available versions of TSDuck.
        bool        download;   // Download the latest version.
        bool        force;      // Force downloads.
        bool        binary;     // With --download, fetch the binaries.
        bool        source;     // With --download, feth the source code instead of the binaries.
        bool        upgrade;    // Upgrade TSDuck to the latest version.
        ts::UString name;       // Use the specified version, not the latest one.
        ts::UString out_dir;    // Output directory for downloaded files.
#endif
    };
}

Options::Options(int argc, char *argv[]) :

#if !defined(TS_NO_GITHUB)
    Args(u"Check version, download and upgrade TSDuck", u"[options]"),
    current(false),
    integer(false),
    extensions(false),
    latest(false),
    check(false),
    all(false),
    download(false),
    force(false),
    binary(false),
    source(false),
    upgrade(false),
    name(),
    out_dir()
#else
    Args(u"Display TSDuck version and extensions", u"[options]"),
    current(true),
    integer(false),
    extensions(false)
#endif
{
    option(u"extensions", 'e');
    help(u"extensions", u"List all available TSDuck extensions.");

    option(u"integer", 'i');
    help(u"integer",
         u"Display the current version of TSDuck in integer format, suitable for "
         u"comparison in a script. Example: " + ts::VersionInfo::GetVersion(ts::VersionInfo::Format::INTEGER) +
         u" for " + ts::VersionInfo::GetVersion(ts::VersionInfo::Format::SHORT) + u".");

    // The following options are used to detect, download and upgrade new versions of TSDuck.
    // They are disabled when TS_NO_GITHUB is defined. With this macro, TSDuck is unlinked
    // from the generic binary distribution on GitHub. Thus, upgrades can be limited to the
    // repositories of a given Linux distro.

#if !defined(TS_NO_GITHUB)

    option(u"all", 'a');
    help(u"all", u"List all available versions of TSDuck from GitHub.");

    option(u"binary", 'b');
    help(u"binary",
         u"With --download, fetch the binary installers of the latest version. "
         u"This is the default. When --source is specified, specify --binary if you also "
         u"need the binary installers.");

    option(u"check", 'c');
    help(u"check", u"Check if a new version of TSDuck is available from GitHub.");

    option(u"download", 'd');
    help(u"download",
         u"Download the latest version (or the version specified by --name) from "
         u"GitHub. By default, download the binary installers for the current "
         u"operating system and architecture. Specify --source to download the "
         u"source code.");
    option(u"force", 'f');
    help(u"force", u"Force downloads even if a file with same name and size already exists.");

    option(u"latest", 'l');
    help(u"latest", u"Display the latest version of TSDuck from GitHub.");

    option(u"name", 'n', STRING);
    help(u"name", u"version-name", u"Get information for or download from GitHub the specified version, not the latest one.");

    option(u"output-directory", 'o', DIRECTORY);
    help(u"output-directory", u"Output directory for downloaded files (current directory by default).");

    option(u"proxy-host", 0, STRING);
    help(u"proxy-host", u"name", u"Optional proxy host name for Internet access.");

    option(u"proxy-password", 0, STRING);
    help(u"proxy-password", u"string", u"Optional proxy password for Internet access (for use with --proxy-user).");

    option(u"proxy-port", 0, UINT16);
    help(u"proxy-port", u"Optional proxy port for Internet access (for use with --proxy-host).");

    option(u"proxy-user", 0, STRING);
    help(u"proxy-user", u"name", u"Optional proxy user name for Internet access.");

    option(u"source", 's');
    help(u"source", u"With --download, download the source code archive instead of the binary installers.");

    option(u"this", 't');
    help(u"this", u"Display the current version of TSDuck (this executable).");

    option(u"upgrade", 'u');
    help(u"upgrade", u"Upgrade TSDuck to the latest version.");

#endif

    analyze(argc, argv);

    extensions = present(u"extensions");
    integer = present(u"integer");

#if !defined(TS_NO_GITHUB)

    all = present(u"all");
    current = present(u"this");
    latest = present(u"latest");
    check = present(u"check");
    binary = present(u"binary");
    source = present(u"source");
    download = present(u"download") || binary || source;
    force = present(u"force");
    upgrade = present(u"upgrade");
    getValue(name, u"name");
    getValue(out_dir, u"output-directory");

    // Proxy settings.
    ts::WebRequest::SetDefaultProxyHost(value(u"proxy-host"), intValue<uint16_t>(u"proxy-port"));
    ts::WebRequest::SetDefaultProxyUser(value(u"proxy-user"), value(u"proxy-password"));

    // Default download is --source.
    if (download && !binary && !source) {
        binary = true;
    }

    // Filter invalid combinations of options.
    if (all + current + integer + latest + check + !name.empty() > 1) {
        error(u"specify only one of --this --integer --latest --name --check --all");
    }

    // If nothing is specified, default to --this
    if (!all && !integer && !latest && !check && !download && !upgrade && name.empty()) {
        current = true;
    }

    // Check output directory.
    if (!out_dir.empty()) {
        if (!ts::IsDirectory(out_dir)) {
            error(u"directory not found: %s", {out_dir});
        }
        else if (!out_dir.endWith(ts::UString(1, ts::PathSeparator))) {
            // Make sure we can use out_dir directly with a file name.
            out_dir.append(ts::PathSeparator);
        }
    }

#endif

    exitOnError();
}


//----------------------------------------------------------------------------
// List all versions.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool ListAllVersions(Options& opt)
    {
        // Get all releases.
        ts::GitHubReleaseVector rels;
        if (!ts::GitHubRelease::GetAllVersions(rels, u"tsduck", u"tsduck", opt)) {
            return false;
        }

        // In non-verbose mode, simply list the versions in the same order as returned by GitHub.
        if (!opt.verbose()) {
            for (const auto& it : rels) {
                std::cout << it->version() << std::endl;
            }
            return true;
        }

        // Compute column widths.
        const ts::UString versionHeader(u"Version");
        const ts::UString dateHeader(u"Published");
        const ts::UString binariesHeader(u"Binaries");
        const ts::UString downloadsHeader(u"Downloads");
        const ts::UString dlPerDayHeader(u"Downl/day");

        size_t versionWidth = versionHeader.width();
        size_t dateWidth = std::max<size_t>(dateHeader.width(), 10); // "yyyy-mm-dd"
        size_t binariesWidth = binariesHeader.width();
        size_t downloadsWidth = downloadsHeader.width();
        size_t dlPerDayWidth = dlPerDayHeader.width();

        for (const auto& it : rels) {
            versionWidth = std::max(versionWidth, it->version().width());
        }

        // List them all.
        std::cout << versionHeader.toJustifiedLeft(versionWidth) << "  "
                  << dateHeader.toJustifiedLeft(dateWidth) << "  "
                  << binariesHeader.toJustifiedRight(binariesWidth) << "  "
                  << downloadsHeader.toJustifiedRight(downloadsWidth) << "  "
                  << dlPerDayHeader.toJustifiedRight(downloadsWidth) << std::endl
                  << ts::UString(versionWidth, u'-') << "  "
                  << ts::UString(dateWidth, u'-') << "  "
                  << ts::UString(binariesWidth, u'-') << "  "
                  << ts::UString(downloadsWidth, u'-') << "  "
                  << ts::UString(dlPerDayWidth, u'-') << std::endl;

        ts::Time endDate(ts::Time::CurrentUTC());
        for (const auto& it : rels) {
            ts::GitHubRelease::AssetList assets;
            it->getAssets(assets);
            const ts::Time startDate(it->publishDate());
            const int days = int((endDate - startDate) / ts::MilliSecPerDay);
            const int downloads = it->assetDownloadCount();
            const int dlPerDay = days <= 0 ? downloads : downloads / days;
            std::cout << it->version().toJustifiedLeft(versionWidth) << "  "
                      << startDate.format(ts::Time::DATE).toJustifiedLeft(dateWidth) << "  "
                      << ts::UString::Decimal(assets.size()).toJustifiedRight(binariesWidth) << "  "
                      << ts::UString::Decimal(downloads).toJustifiedRight(downloadsWidth) << "  "
                      << ts::UString::Decimal(dlPerDay).toJustifiedRight(dlPerDayWidth) << std::endl;
            endDate = startDate; // for previous version
        }
        return true;
    }
}
#endif


//----------------------------------------------------------------------------
// Display one release.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool DisplayRelease(Options& opt, const ts::GitHubRelease& rel)
    {
        // In non-verbose mode, simply display the version.
        if (!opt.verbose()) {
            std::cout << rel.version() << std::endl;
            return true;
        }

        // Release overview
        std::cout << "Version: " << rel.version() << std::endl
                  << "Description: " << rel.versionName() << std::endl
                  << "Published: " << rel.publishDate().format(ts::Time::DATE) << std::endl
                  << "Downloads: " << rel.assetDownloadCount() << std::endl
                  << "Source code: " << rel.sourceURL() << std::endl;

        // Binary assets.
        ts::GitHubRelease::AssetList assets;
        rel.getAssets(assets);

        if (assets.empty()) {
            std::cout << "No binary package available" << std::endl;
        }
        else {
            std::cout << "Binary packages:" << std::endl;
            size_t applyCount = 0;
            for (const auto& it : assets) {
                if (ts::GitHubRelease::IsPlatformAsset(it.name)) {
                    ++applyCount;
                }
                std::cout << "  " << it.name << " (" << ts::UString::HumanSize(it.size);
                if (it.downloadCount > 0) {
                    std::cout << ts::UString::Format(u", %'d downloads", {it.downloadCount});
                }
                std::cout << ")" << std::endl;
            }
            if (applyCount > 0) {
                std::cout << "Available downloads for your system:" << std::endl;
                for (const auto& it : assets) {
                    if (ts::GitHubRelease::IsPlatformAsset(it.name)) {
                        std::cout << "  " << it.url << std::endl;
                    }
                }
            }
        }

        return true;
    }
}
#endif


//----------------------------------------------------------------------------
// Download a file.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool DownloadFile(Options& opt, const ts::UString& url, const ts::UString& file, int64_t size)
    {
        // Without --force, don't download when a file exists with same size.
        if (!opt.force) {
            // If the size is unknown, do not download again if the file is not empty, trust the size.
            const int64_t fileSize = ts::GetFileSize(file);
            if ((size == 0 && fileSize > 0) || (size > 0 && fileSize == size)) {
                if (opt.verbose()) {
                    std::cout << "File already downloaded: " << file << std::endl;
                }
                return true;
            }
        }

        // Download the file.
        ts::WebRequest web(opt);
        std::cout << "Downloading " << file << " ..." << std::endl;
        return web.downloadFile(url, file);
    }
}
#endif


//----------------------------------------------------------------------------
// Download a release.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool DownloadRelease(Options& opt, const ts::GitHubRelease& rel, bool forceBinary)
    {
        bool success = true;

        // Download source package if required.
        if (opt.source) {
            // Size of source archive is unknown, not sent by GitHub.
            // This is probably because source archives are generated on the
            // fly and it is difficult to predict the size of a compressed file.
            success = DownloadFile(opt, rel.sourceURL(), opt.out_dir + rel.sourceFileName(), 0);
        }

        // Get assets for this platform.
        if (opt.binary || forceBinary) {

            ts::GitHubRelease::AssetList assets;
            rel.getPlatformAssets(assets);

            if (assets.empty()) {
                if (opt.verbose()) {
                    std::cout << "There is no binary package for this release." << std::endl;
#if defined(TS_MAC)
                    std::cout << "On macOS, use Homebrew (\"brew upgrade tsduck\")." << std::endl;
#endif
                }
            }
            else {
                for (const auto& it : assets) {
                    success = DownloadFile(opt, it.url, opt.out_dir + it.name, it.size) && success;
                }
            }
        }

        return success;
    }
}
#endif


//----------------------------------------------------------------------------
// Run an upgrade command.
// Do not stay in current tsversion process since the upgrade command
// will upgrade its executable file.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool RunUpgradeCommand(Options& opt, const ts::UString& command, bool needPrivilege)
    {
        ts::UString cmd(command);

        // Use a privileged command from an non-privileged process ?
        if (needPrivilege && !ts::IsPrivilegedUser()) {
            #if defined(TS_UNIX)
                // Same command using sudo.
                cmd.insert(0, u"sudo ");
            #elif defined(TS_WINDOWS)
                // On Windows, use a completely different method.
                std::cout << "Running: " << cmd << std::endl;
                return ts::WinCreateElevatedProcess(cmd, false, opt);
            #endif
        }
        std::cout << "Running: " << cmd << std::endl;

        // Run the upgrade command and exit current process.
        ts::ForkPipe process;
        bool success = process.open(cmd, ts::ForkPipe::EXIT_PROCESS, 0, CERR, ts::ForkPipe::KEEP_BOTH, ts::ForkPipe::STDIN_PARENT);
        process.close(NULLREP);
        return success;
    }
}
#endif


//----------------------------------------------------------------------------
// Upgrade to a release.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool UpgradeRelease(Options& opt, const ts::GitHubRelease& rel)
    {
        // Download binaries if not yet done.
        if (!DownloadRelease(opt, rel, true)) {
            return false;
        }

        // Get local asset files for this platform.
        ts::GitHubRelease::AssetList assets;
        rel.getPlatformAssets(assets);
        ts::UStringList files;
        for (const auto& it : assets) {
            files.push_back(opt.out_dir + it.name);
        }

        // Get system info to determine which command to run.
        const ts::SysInfo& sys(*ts::SysInfo::Instance());
        const ts::UString sysName(sys.systemName().empty() ? u"this system" : sys.systemName());

        if (files.empty() && !sys.isMacOS()) {
            opt.error(u"no binary installer available for %s", {sysName});
            return false;
        }

        if (sys.isWindows()) {
            // On Windows, there should be only one installer.
            if (files.size() != 1) {
                opt.error(u"found %d installers for this version, manually run one of: %s", {files.size(), ts::UString::Join(files, u" ")});
                return false;
            }
            // We require a privileged execution.
            // The execution is asynchronous. We exit tsversion immediately after launching the installer.
            // We can't wait for the completion of the installer since it will replace tsversion.exe and
            // tsduck.dll, which would be locked if tsversion is still executing.
            return RunUpgradeCommand(opt, files.front(), true);
        }
        else if (sys.isMacOS()) {
            return RunUpgradeCommand(opt, u"brew upgrade tsduck", false);
        }
        else if (sys.isFedora() || sys.isRedHat()) {
            return RunUpgradeCommand(opt, u"rpm -Uvh " + ts::UString::Join(files, u" "), true);
        }
        else if (sys.isUbuntu() || sys.isDebian() || sys.isRaspbian()) {
            return RunUpgradeCommand(opt, u"dpkg -i " + ts::UString::Join(files, u" "), true);
        }
        else {
            opt.error(u"don't know how to upgrade on %s, rebuild from sources", {sysName});
            return false;
        }
    }
}
#endif


//----------------------------------------------------------------------------
// Check the availability of a new version.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool CheckNewVersion(Options& opt, const ts::GitHubRelease& rel)
    {
        const ts::UString current(ts::VersionInfo::GetVersion());
        const ts::UString remote(rel.version());
        const int comp = ts::VersionInfo::CompareVersions(current, remote);

        // Cases where there is no new version.
        if (comp == 0) {
            std::cout << "Your version " << current << " is the latest one" << std::endl;
            return true;
        }
        else if (comp > 0) {
            std::cout << "Your version " << current << " is more recent than " << remote << " online" << std::endl;
            return true;
        }

        // We have a new version, get available assets for this platform.
        ts::GitHubRelease::AssetList assets;
        rel.getPlatformAssets(assets);

        // Display new version.
        std::cout << "New version " << remote << " is available (yours is " << current << ")" << std::endl;
        if (opt.verbose() && !assets.empty()) {
            std::cout << "Available downloads for your system:" << std::endl;
            for (const auto& it : assets) {
                std::cout << "  " << it.url << std::endl;
            }
        }

        // Download and/or upgrade.
        if (opt.upgrade) {
            return UpgradeRelease(opt, rel);
        }
        if (opt.download) {
            return DownloadRelease(opt, rel, false);
        }
        return true;
    }
}
#endif


//----------------------------------------------------------------------------
// Process one version.
//----------------------------------------------------------------------------

#if !defined(TS_NO_GITHUB)
namespace {
    bool ProcessVersion(Options& opt)
    {
        // By convention, TSDuck use tag named "vX.Y-Z" for version X.Y-Z.
        // An empty tag name specifies the latest version.
        ts::UString tagName;
        if (!opt.name.empty()) {
            tagName = u"v" + opt.name;
        }

        // Get information about the release.
        const ts::GitHubRelease rel(u"tsduck", u"tsduck", tagName, opt);
        if (!rel.isValid()) {
            return false;
        }
        if (rel.version().empty()) {
            opt.error(u"unable to identify version");
            return false;
        }

        // Display release name if nothing more to do.
        if (!opt.check && !opt.download && !opt.upgrade) {
            return DisplayRelease(opt, rel);
        }

        // Check existence of more recent version.
        // --upgrade if done only on new versions.
        if (opt.check || opt.upgrade) {
            return CheckNewVersion(opt, rel);
        }

        // Download a version (without checking).
        if (opt.download) {
            return DownloadRelease(opt, rel, false);
        }

        return true;
    }
}
#endif


//----------------------------------------------------------------------------
// Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    Options opt(argc, argv);
    bool success = true;

    if (opt.extensions) {
        // Display list of available extensions.
        // The returned string is either empty or ends with a new-line.
        std::cout << ts::DuckExtensionRepository::Instance()->listExtensions(opt);
    }
    else if (opt.integer) {
        // Display current version in integer format.
        std::cout << ts::VersionInfo::GetVersion(ts::VersionInfo::Format::INTEGER) << std::endl;
    }
    else if (opt.current) {
        // Display current version.
        std::cout << ts::VersionInfo::GetVersion(opt.verbose() ? ts::VersionInfo::Format::LONG : ts::VersionInfo::Format::SHORT) << std::endl;
    }

#if !defined(TS_NO_GITHUB)

    else if (opt.all) {
        success = ListAllVersions(opt);
    }
    else {
        success = ProcessVersion(opt);
    }

#endif

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
