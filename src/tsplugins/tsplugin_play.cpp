//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  Transport stream processor shared library:
//  Play resulting TS in any supported media player, as found on the system.
//
//----------------------------------------------------------------------------

#include "tsPlugin.h"
#include "tsForkPipe.h"
#include "tsStringUtils.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;

#if defined(__windows)
#include "tsRegistryUtils.h"
#endif

// Pipe buffer size is used on Windows only.
#define PIPE_BUFFER_SIZE 65536


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PlayPlugin: public OutputPlugin
    {
    public:
        // Implementation of plugin API
        PlayPlugin (TSP*);
        virtual bool start();
        virtual bool stop();
        virtual BitRate getBitrate() {return 0;}
        virtual bool send (const TSPacket*, size_t);

    private:
        bool     _use_mplayer;
        bool     _use_xine;
        ForkPipe _pipe;

        // Search a file in a search path. Return true is found
        bool searchInPath (std::string& result, const StringVector& path, const std::string name);

        // Inaccessible operations
        PlayPlugin() = delete;
        PlayPlugin(const PlayPlugin&) = delete;
        PlayPlugin& operator=(const PlayPlugin&) = delete;
    };
}

TSPLUGIN_DECLARE_VERSION
TSPLUGIN_DECLARE_OUTPUT(ts::PlayPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PlayPlugin::PlayPlugin (TSP* tsp_) :
    OutputPlugin(tsp_, "Play output TS on any supported media player in the system.", "[options]"),
    _use_mplayer(false),
    _use_xine(false),
    _pipe()
{
    option ("mplayer", 'm');
    option ("xine",    'x');

    setHelp ("Options:\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
#if !defined(__windows)
             "\n"
             "  -m\n"
             "  --mplayer\n"
             "      Use mplayer for rendering. The default is to look for vlc, mplayer and\n"
             "      xine, in this order, and use the first available one.\n"
#endif
             "\n"
             "  --version\n"
             "      Display the version number.\n"
#if !defined(__windows)
             "\n"
             "  -x\n"
             "  --xine\n"
             "      Use xine for rendering. The default is to look for vlc, mplayer and\n"
             "      xine, in this order, and use the first available one.\n"
#endif
             );
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::PlayPlugin::stop()
{
    return _pipe.close (*tsp);
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::PlayPlugin::send (const TSPacket* buffer, size_t packet_count)
{
    return _pipe.write (buffer, PKT_SIZE * packet_count, *tsp);
}


//----------------------------------------------------------------------------
// Search a file in a search path. Return empty string if not found
//----------------------------------------------------------------------------

bool ts::PlayPlugin::searchInPath (std::string& result, const StringVector& path, const std::string name)
{
    for (StringVector::const_iterator it = path.begin(); it != path.end(); ++it) {
        if (!it->empty()) {
            result = *it + PathSeparator + name;
            tsp->debug ("looking for " + result);
            if (FileExists (result)) {
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
    _use_mplayer = present ("mplayer");
    _use_xine = present ("xine");
    if (_use_mplayer && _use_xine) {
        tsp->error ("--mplayer (-m) and --xine (-x) are mutually exclusive");
        return false;
    }

    // Command to execute will be built here
    std::string command;

#if defined (__windows)

    // On Windows, VLC is the only known media player that can read an MPEG
    // transport stream on its standard input. Try to locate vlc.exe using
    // various means.

    // Get environment path
    StringVector search_path;
    GetEnvironmentPath (search_path, "Path");

    // Look into some registry location
    std::string ent = GetRegistryValue ("HKLM\\SOFTWARE\\VideoLAN\\VLC", "InstallDir");
    if (!ent.empty()) {
        search_path.push_back (ent);
    }
    ent = GetRegistryValue ("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VLC media player", "UninstallString");
    if (!ent.empty()) {
        search_path.push_back (DirectoryName (ent));
    }

    // Add default installation location
    search_path.push_back ("C:\\Program Files\\VideoLAN\\VLC");

    // Then search vlc.exe in these locations
    std::string exec;
    if (searchInPath (exec, search_path, "vlc.exe")) {
        command = "\"" + exec + "\" -";
    }
    else {
        tsp->error ("VLC not found, install VideoLAN VLC media player, see http://www.videolan.org/vlc/");
        return false;
    }

#else // UNIX

    // Get environment path.
    StringVector search_path;
    GetEnvironmentPath (search_path, "PATH");

    // Executable names for various players
    const char* const vlc_exec = "vlc";
    const char* const mplayer_exec = "mplayer";
    const char* const xine_exec = "xine";

    // Options to read TS on stdin for various players
    const char* const vlc_opts = "-";
    const char* const mplayer_opts = "-demuxer +mpegts -";
    const char* const xine_opts = "stdin:/#demux:mpeg-ts";

    // Search known media players
    std::string exec;
    const char* opts = "";

    if (_use_mplayer) {
        opts = mplayer_opts;
        if (!searchInPath (exec, search_path, mplayer_exec)) {
            tsp->error ("mplayer not found in PATH");
            return false;
        }
    }
    else if (_use_xine) {
        opts = xine_opts;
        if (!searchInPath (exec, search_path, xine_exec)) {
            tsp->error ("xine not found in PATH");
            return false;
        }
    }
    else if (searchInPath (exec, search_path, vlc_exec)) {
        opts = vlc_opts;
    }
    else if (searchInPath (exec, search_path, mplayer_exec)) {
        opts = mplayer_opts;
    }
    else if (searchInPath (exec, search_path, xine_exec)) {
        opts = xine_opts;
    }
    else {
        tsp->error ("no supported media player was found");
        return false;
    }

    command = "\"" + exec + "\" " + opts;

#endif

    // Create pipe & process
    tsp->verbose ("using media player command: " + command);
    _pipe.setIgnoreAbort (false);
    return _pipe.open (command, true, PIPE_BUFFER_SIZE, *tsp);
}
