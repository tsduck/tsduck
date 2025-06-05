//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  Transport stream processor shared library:
//  Play resulting TS in any supported media player, as found on the system.
//
//----------------------------------------------------------------------------

#include "tsPluginRepository.h"
#include "tsTSForkPipe.h"
#include "tsFileUtils.h"
#include "tsRegistry.h"

// Pipe buffer size is used on Windows only.
#define PIPE_BUFFER_SIZE 65536


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PlayPlugin: public OutputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PlayPlugin);
    public:
        // Implementation of plugin API
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool isRealTime() override {return true;}
        virtual bool send(const TSPacket*, const TSPacketMetadata*, size_t) override;

    private:
        bool       _use_mplayer = false;
        bool       _use_ffplay = false;
        bool       _use_xine = false;
        TSForkPipe _pipe {};

        // Search a file in a search path. Return true is found
        bool searchInPath(UString& result, const UStringVector& path, const UString& name);
    };
}

TS_REGISTER_OUTPUT_PLUGIN(u"play", ts::PlayPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PlayPlugin::PlayPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Play output TS on any supported media player in the system", u"[options]")
{
#if !defined(TS_WINDOWS)

    option(u"mplayer", 'm');
    help(u"mplayer",
         u"Use mplayer for rendering. "
         u"The default is to look for vlc, mplayer, ffplay, xine, in this order, and use the first available one.");

    option(u"ffplay", 'f');
    help(u"ffplay",
         u"Use ffplay (part of ffmpeg) for rendering. "
         u"The default is to look for vlc, mplayer, ffplay, xine, in this order, and use the first available one.");

    option(u"xine", 'x');
    help(u"xine",
         u"Use xine for rendering. "
         u"The default is to look for vlc, mplayer, ffplay, xine, in this order, and use the first available one.");

#endif
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::PlayPlugin::stop()
{
    return _pipe.close(*this);
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::PlayPlugin::send(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count)
{
    return _pipe.writePackets(buffer, pkt_data, packet_count, *this);
}


//----------------------------------------------------------------------------
// Search a file in a search path. Return empty string if not found
//----------------------------------------------------------------------------

bool ts::PlayPlugin::searchInPath(UString& result, const UStringVector& path, const UString& name)
{
    for (const auto& file : path) {
        if (!file.empty()) {
            result = file + fs::path::preferred_separator + name;
            debug(u"looking for %s", result);
            if (fs::exists(result)) {
                return true;
            }
        }
    }
    return false;
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::PlayPlugin::start()
{
    // Get option values
#if !defined(TS_WINDOWS)
    _use_mplayer = present(u"mplayer");
    _use_ffplay = present(u"ffplay");
    _use_xine = present(u"xine");
    if (_use_mplayer + _use_xine + _use_ffplay > 1) {
        error(u"--mplayer, --ffplay and --xine are mutually exclusive");
        return false;
    }
#endif

    // Command to execute will be built here
    UString command;

    // Get environment path
    UStringVector search_path;
    GetEnvironmentPath(search_path, PATH_ENVIRONMENT_VARIABLE);

#if defined(TS_WINDOWS)

    // On Windows, VLC is the only known media player that can read an MPEG transport stream on its standard input.
    // Try to locate vlc.exe using various means.

    // Look into some registry location
    UString ent = Registry::GetValue(u"HKLM\\SOFTWARE\\VideoLAN\\VLC", u"InstallDir");
    if (!ent.empty()) {
        search_path.push_back(ent);
    }
    ent = Registry::GetValue(u"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VLC media player", u"UninstallString");
    if (!ent.empty()) {
        search_path.push_back(DirectoryName(ent));
    }

    // Add default installation locations
    search_path.push_back(u"C:\\Program Files\\VideoLAN\\VLC");
    search_path.push_back(u"C:\\Program Files (x86)\\VideoLAN\\VLC");

    // Then search vlc.exe in these locations
    UString exec;
    if (searchInPath(exec, search_path, u"vlc.exe")) {
        // Enclose the executable path with quotes and use "-" as parameter (meaning standard input).
        command = u"\"" + exec + u"\" -";
    }
    else {
        error(u"VLC not found, install VideoLAN VLC media player, see http://www.videolan.org/vlc/");
        return false;
    }

#else // UNIX

    // On macOS, additional applications are installed outside /usr because of system integrity protection.
#if defined(TS_MAC)
    // On Apple Silicon Mac's, Homebrew is installed in /opt/homebrew.
    #if defined(TS_ARM64)
        search_path.push_back(u"/opt/homebrew/bin");
    #endif
    search_path.push_back(u"/usr/local/bin");
#endif

    // Executable names for various players
    static const UChar vlc_exec[] = u"vlc";
    static const UChar mplayer_exec[] = u"mplayer";
    static const UChar ffplay_exec[] = u"ffplay";
    static const UChar xine_exec[] = u"xine";

    // On macOS, the applications are installed elsewhere.
#if defined(TS_MAC)
    static const UChar mac_vlc_exec[] = u"/Applications/VLC.app/Contents/MacOS/VLC";
#endif

    // Options to read TS on stdin for various players
    static const UChar vlc_opts[] = u"- --play-and-exit";
    static const UChar mplayer_opts[] = u"-demuxer +mpegts -";
    static const UChar ffplay_opts[] = u"-loglevel error -autoexit -f mpegts -";
    static const UChar xine_opts[] = u"stdin:/#demux:mpeg-ts";

    // Search known media players
    UString exec;
    const UChar* opts = u"";

    if (_use_mplayer) {
        opts = mplayer_opts;
        if (!searchInPath(exec, search_path, mplayer_exec)) {
            error(u"mplayer not found in PATH");
            return false;
        }
    }
    else if (_use_ffplay) {
        opts = ffplay_opts;
        if (!searchInPath(exec, search_path, xine_exec)) {
            error(u"ffplay not found in PATH");
            return false;
        }
    }
    else if (_use_xine) {
        opts = xine_opts;
        if (!searchInPath(exec, search_path, xine_exec)) {
            error(u"xine not found in PATH");
            return false;
        }
    }
#if defined(TS_MAC)
    else if (fs::exists(mac_vlc_exec)) {
        exec = mac_vlc_exec;
        opts = vlc_opts;
    }
#endif
    else if (searchInPath(exec, search_path, vlc_exec)) {
        opts = vlc_opts;
    }
    else if (searchInPath(exec, search_path, mplayer_exec)) {
        opts = mplayer_opts;
    }
    else if (searchInPath(exec, search_path, ffplay_exec)) {
        opts = ffplay_opts;
    }
    else if (searchInPath(exec, search_path, xine_exec)) {
        opts = xine_opts;
    }
    else {
        error(u"no supported media player was found");
        return false;
    }

    command = u"\"" + exec + u"\" " + opts;

#endif

    // Create pipe & process
    verbose(u"using media player command: %s", command);
    _pipe.setIgnoreAbort(false);
    return _pipe.open(command, ForkPipe::SYNCHRONOUS, PIPE_BUFFER_SIZE, *this, ForkPipe::KEEP_BOTH, ForkPipe::STDIN_PIPE);
}
