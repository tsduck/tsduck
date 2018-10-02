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
//!  Transport stream file input
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Transport Stream file input.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSFileInput
    {
    public:
        //!
        //! Default constructor.
        //!
        TSFileInput();

        //!
        //! Destructor.
        //!
        virtual ~TSFileInput();

        //!
        //! Open the file.
        //! @param [in] filename File name. If empty, use standard input.
        //! Must be a regular file is @a repeat_count is not 1 or if
        //! @a start_offset is not zero.
        //! @param [in] repeat_count Reading packets loops back after end of
        //! file until all repeat are done. If zero, infinitely repeat.
        //! @param [in] start_offset Offset in bytes from the beginning of the file
        //! where to start reading packets at each iteration.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(const UString& filename, size_t repeat_count, uint64_t start_offset, Report& report);

        //!
        //! Open the file in rewindable mode.
        //! The file must be a rewindable file, eg. not a pipe.
        //! There is no repeat count, rewind must be done explicitly.
        //! @param [in] filename File name. If empty, use standard input.
        //! @param [in] start_offset Offset in bytes from the beginning of the file
        //! where to start reading packets.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //! @see rewind()
        //! @see seek()
        //!
        bool open(const UString& filename, uint64_t start_offset, Report& report);

        //!
        //! Check if the file is open.
        //! @return True if the file is open.
        //!
        bool isOpen() const
        {
            return _is_open;
        }

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
        //! Close the file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Read TS packets.
        //! If the file file was opened with a @a repeat_count different from 1,
        //! reading packets transparently loops back at end if file.
        //! @param [out] buffer Address of reception packet buffer.
        //! @param [in] max_packets Size of @a buffer in packets.
        //! @param [in,out] report Where to report errors.
        //! @return The actual number of read packets. Returning zero means
        //! error or end of file repetition.
        //!
        size_t read(TSPacket* buffer, size_t max_packets, Report& report);

        //!
        //! Abort any currenly read operation in progress.
        //! The file is left in a broken state and can be only closed.
        //!
        void abortRead();

        //!
        //! Rewind the file.
        //! The file must have been opened in rewindable mode.
        //! If the file file was opened with a @a start_offset different from 0,
        //! rewinding the file means restarting at this @a start_offset.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool rewind(Report& report)
        {
            return seek(0, report);
        }

        //!
        //! Seek the file at a specified packet index.
        //! The file must have been opened in rewindable mode.
        //! @param [in] packet_index Seek the file to this specified packet index
        //! (plus the previously specified @a start_offset).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool seek(PacketCounter packet_index, Report& report);

        //!
        //! Get the number of read packets.
        //! @return The number of read packets.
        //!
        PacketCounter getPacketCount() const
        {
            return _total_packets;
        }

    protected:
        UString       _filename;      //!< Input file name.
        PacketCounter _total_packets; //!< Total read packets.

    private:
        size_t        _repeat;        //!< Repeat count (0 means infinite)
        size_t        _counter;       //!< Current repeat count
        uint64_t      _start_offset;  //!< Initial byte offset in file
        volatile bool _is_open;       //!< Check if file is actually open
        int           _severity;      //!< Severity level for error reporting
        volatile bool _at_eof;        //!< End of file has been reached
        bool          _rewindable;    //!< Opened in rewindable mode
#if defined(TS_WINDOWS)
        ::HANDLE      _handle;        //!< File handle
#else
        int           _fd;            //!< File descriptor
#endif

        // Inaccessible operations
        TSFileInput(const TSFileInput&) = delete;
        TSFileInput& operator=(const TSFileInput&) = delete;

        // Internal methods
        bool openInternal(Report& report);
        bool seekInternal(uint64_t, Report& report);
    };
}
