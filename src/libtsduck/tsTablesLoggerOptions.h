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
//  Options for the class TablesLogger.
//
//----------------------------------------------------------------------------

#pragma once
#include "tsArgs.h"
#include "tsMPEG.h"
#include "tsCASFamily.h"

namespace ts {

    class TSDUCKDLL TablesLoggerOptions: public Args
    {
    public:
        // Constructor.
        TablesLoggerOptions (const std::string& description = "",
                             const std::string& syntax = "",
                             const std::string& help = "",
                             int flags = 0);

        // Type of logging destination
        enum Mode {TEXT, BINARY, UDP};

        // Public fields, by options.
        Mode         mode;          // Type of destination
        std::string  destination;   // Destination name (file, host, etc)
        bool         multi_files;   // Multiple binary output files (one per section)
        bool         flush;         // Flush output file
        std::string  udp_local;     // Name of outgoing local address (empty if unspecified)
        int          udp_ttl;       // Time-to-live socket option
        bool         all_sections;  // Collect all sections, as they appear
        uint32_t       max_tables;    // Max number of tables to dump
        bool         raw_dump;      // Raw dump of section, no interpretation
        uint32_t       raw_flags;     // Dump flags in raw mode
        bool         time_stamp;    // Display time stamps with each table
        bool         packet_index;  // Display packet index with each table
        CASFamily    cas;           // CAS family
        bool         diversified;   // Payload must be diversified
        bool         logger;        // Table logger
        size_t       log_size;      // Size of table to log
        bool         negate_tid;    // Negate tid filter (exclude selected tids)
        bool         negate_tidext; // Negate tidext filter (exclude selected tidexts)
        PIDSet       pid;           // PID values to filter
        bool         add_pmt_pids;  // Add PMT PID's when one is found
        std::set<uint8_t>  tid;       // TID values to filter
        std::set<uint16_t> tidext;    // TID-ext values to filter
        std::set<uint32_t> emm_group; // Shared EMM group numbers to filter
        std::set<uint32_t> emm_ua;    // Individual EMM unique addresses to filter

        // Some default values
        static const size_t DEFAULT_LOG_SIZE = 8;

        // Check if standard output shall be used
        bool useCout() const {return mode == TEXT && destination.empty();}

        // Overriden methods.
        void setHelp (const std::string& help);
        virtual bool analyze (int argc, char* argv[]);
        virtual bool analyze (const std::string& app_name, const StringVector& arguments);

        // Get option values (the public fields) after analysis of another
        // ts::Args object defining the same options.
        void getOptions (Args&);

    private:
        // Inaccessible operations
        virtual bool analyze(const char* app_name, const char* arg1, ...);
    };
}
