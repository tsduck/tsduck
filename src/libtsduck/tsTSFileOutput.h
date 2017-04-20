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
//!  Transport stream file output
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsReportInterface.h"

namespace ts {

    class TSDUCKDLL TSFileOutput
    {
    public:
        // Constructor / destructor
        TSFileOutput () : _is_open (false), _severity (Severity::Error), _total_packets (0) {}
        virtual ~TSFileOutput ();

        // If filename is empty, use standard output.
        bool open (const std::string& filename, bool append, bool keep, ReportInterface&);
        bool close (ReportInterface&);
        bool write (const TSPacket*, size_t packet_count, ReportInterface&);

        // Check if file is open.
        bool isOpen() const {return _is_open;}

        // Get/set severity level for error reporting. The default is Error.
        int getErrorSeverityLevel() const {return _severity;}
        void setErrorSeverityLevel (int level) {_severity = level;}

        // Get file name
        std::string getFileName() const {return _filename;}

        // Return the number of written packets
        PacketCounter getPacketCount() const {return _total_packets;}

    private:
        std::string   _filename;      // Output file name
        bool          _is_open;       // Check if file is actually open
        int           _severity;      // Severity level for error reporting
        PacketCounter _total_packets; // Total written packets
#if defined (__windows)
        ::HANDLE      _handle;        // File handle
#else
        int           _fd;            // File descriptor
#endif
        // Inaccessible operations
        TSFileOutput(const TSFileOutput&) = delete;
        TSFileOutput& operator=(const TSFileOutput&) = delete;
    };
}
