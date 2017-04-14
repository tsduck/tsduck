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
//  Transport stream file input
//
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsReportInterface.h"

namespace ts {

    class TSDUCKDLL TSFileInput
    {
    public:
        // Constructor / destructor
        TSFileInput () : _total_packets (0), _is_open (false), _severity (Severity::Error) {}
        virtual ~TSFileInput ();

        // Open file.
        // If filename is empty, use standard input.
        // If repeat_count != 1, reading packets loops back to the start_offset
        // until all repeat are done. If repeat_count == 0, infinite repeat.
        bool open(const std::string& filename, size_t repeat_count, uint64_t start_offset, ReportInterface&);

        // Open file in a rewindable mode (must be a rewindable file, eg. not a pipe).
        // There is no repeat count, rewind must be done explicitely.
        // If filename is empty, use standard input.
        bool open(const std::string& filename, uint64_t start_offset, ReportInterface&);

        // Check if file is open.
        bool isOpen() const {return _is_open;}

        // Get/set severity level for error reporting. The default is Error.
        int getErrorSeverityLevel() const {return _severity;}
        void setErrorSeverityLevel(int level) {_severity = level;}

        // Get file name
        std::string getFileName() const {return _filename;}

        // Close file.
        bool close(ReportInterface&);

        // Read TS packets. Return the actual number of read packets.
        // Returning zero means error or end of file repetition.
        size_t read (TSPacket*, size_t max_packets, ReportInterface&);

        // Rewind the file to the previously specified start_offset.
        // The file must have been open in rewindable mode.
        bool rewind (ReportInterface& report) {return seek (0, report);}

        // Seek the file to the specified packet_index (plus the previously specified start_offset).
        // The file must have been open in rewindable mode.
        bool seek (PacketCounter, ReportInterface&);

        // Return the number of read packets
        PacketCounter getPacketCount() const {return _total_packets;}

    protected:
        std::string   _filename;      // Input file name
        PacketCounter _total_packets; // Total read packets

    private:
        size_t   _repeat;        // Repeat count (0 means infinite)
        size_t   _counter;       // Current repeat count
        uint64_t _start_offset;  // Initial byte offset in file
        bool     _is_open;       // Check if file is actually open
        int      _severity;      // Severity level for error reporting
        bool     _at_eof;        // End of file has been reached
        bool     _rewindable;    // Opened in rewindable mode
#if defined (__windows)
        ::HANDLE _handle;        // File handle
#else
        int      _fd;            // File descriptor
#endif

        // Inaccessible operations
        TSFileInput(const TSFileInput&) = delete;
        TSFileInput& operator=(const TSFileInput&) = delete;

        // Internal methods
        bool openInternal(ReportInterface&);
        bool seekInternal(uint64_t, ReportInterface&);
    };
}
