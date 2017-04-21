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
//!
//!  @file
//!  Transport stream processor command-line options
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"

namespace ts {
    //!
    //! Transport Stream Processor (tsp) namespace
    //!
    namespace tsp {

        class Options: public Args
        {
        public:
            // Constructor from command line options
            Options(int argc, char *argv[]);

            // Each plugin has one of the following types
            enum PluginType {INPUT, OUTPUT, PROCESSOR};

            // Displayable name
            static std::string PluginTypeName (PluginType);

            // Plugin options
            struct PluginOptions
            {
                // Public fields
                PluginType   type;
                std::string  name;
                StringVector args;

                // Display the content of the object to a stream
                std::ostream& display (std::ostream& strm, int indent = 0) const;
            };
            typedef std::vector <PluginOptions> PluginOptionsVector;

            // Option values
            bool          verbose;         // Verbose output
            int           debug;           // Debug level
            bool          timed_log;       // Add time stamps in log messages
            bool          list_proc;       // List processors
            bool          monitor;         // Run a resource monitoring thread
            bool          ignore_jt;       // Ignore "joint termination" options in plugins
            size_t        bufsize;         // Buffer size
            size_t        max_flush_pkt;   // Max processed packets before flush
            size_t        max_input_pkt;   // Max packets per input operation
            size_t        instuff_nullpkt; // Add input stuffing: add nullpkt null...
            size_t        instuff_inpkt;   // ... packets every inpkt input packets
            BitRate       bitrate;         // Fixed input bitrate
            MilliSecond   bitrate_adj;     // Bitrate adjust interval
            PluginOptions input;           // Input plugin
            PluginOptions output;          // Output plugin
            PluginOptionsVector plugins;   // List of packet processor plugins

            // Display the content of the object to a stream
            std::ostream& display (std::ostream& strm, int indent = 0) const;

        private:
            // Search the next plugin option.
            // Start searching at index + 1.
            // Return plugin option index (== argc if not found)
            static int nextProcOpt (int argc, char *argv[], int index, PluginType& type);
        };
    }
}

// Display operator
inline std::ostream& operator<< (std::ostream& strm, const ts::tsp::Options& opt)
{
    return opt.display (strm);
}
