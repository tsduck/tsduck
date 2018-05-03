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
//!
//!  @file
//!  Transport stream file output
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Transport Stream file output.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSFileOutput
    {
    public:
        //!
        //! Default constructor.
        //!
        TSFileOutput();

        //!
        //! Destructor.
        //!
        virtual ~TSFileOutput();

        //!
        //! Open or create the file.
        //! @param [in] filename File name. If empty, use standard output.
        //! @param [in] append Append packets to an existing file.
        //! @param [in] keep Keep previous file with same name. Fail if it already exists.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        virtual bool open(const UString& filename, bool append, bool keep, Report& report);

        //!
        //! Close the file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Write TS packets to the file.
        //! @param [in] buffer Address of first packet to write.
        //! @param [in] packet_count Number of packets to write.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool write(const TSPacket* buffer, size_t packet_count, Report& report);

        //!
        //! Check if the file is open.
        //! @return True if the file is open.
        //!
        bool isOpen() const {return _is_open;}

        //!
        //! Get the severity level for error reporting.
        //! @return The severity level for error reporting.
        //!
        int getErrorSeverityLevel() const
        {
            return _severity;
        }

        //!
        //! Set the severity level for error reporting.
        //! @param [in] level The severity level for error reporting. The default is Error.
        //!
        void setErrorSeverityLevel(int level)
        {
            _severity = level;
        }

        //!
        //! Get the file name.
        //! @return The file name.
        //!
        UString getFileName() const
        {
            return _filename;
        }

        //!
        //! Get the number of written packets.
        //! @return The number of written packets.
        //!
        PacketCounter getPacketCount() const
        {
            return _total_packets;
        }

    private:
        UString       _filename;      // Output file name
        bool          _is_open;       // Check if file is actually open
        int           _severity;      // Severity level for error reporting
        PacketCounter _total_packets; // Total written packets
#if defined(TS_WINDOWS)
        ::HANDLE      _handle;        // File handle
#else
        int           _fd;            // File descriptor
#endif
        // Inaccessible operations
        TSFileOutput(const TSFileOutput&) = delete;
        TSFileOutput& operator=(const TSFileOutput&) = delete;
    };
}
