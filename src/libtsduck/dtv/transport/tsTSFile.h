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
//!
//!  @file
//!  Transport stream file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacketStream.h"
#include "tsAbstractReadStreamInterface.h"
#include "tsAbstractWriteStreamInterface.h"
#include "tsEnumUtils.h"

namespace ts {

    class TSPacketMetadata;

    //!
    //! Transport stream file, input and/or output.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSFile :
        public TSPacketStream,
        private AbstractReadStreamInterface,
        private AbstractWriteStreamInterface
    {
    public:
        //!
        //! Default constructor.
        //!
        TSFile();

        //!
        //! Destructor.
        //!
        virtual ~TSFile() override;

        //!
        //! Copy constructor.
        //! Only the configuration is copied (name, repetition, etc.)
        //! @param [in] other Other instance to copy.
        //!
        TSFile(const TSFile& other);

        //!
        //! Move constructor.
        //! The full state is moved.
        //! @param [in,out] other Other instance to move. Closed on return.
        //!
        TSFile(TSFile&& other) noexcept;

        //!
        //! Open the file for read.
        //! @param [in] filename File name. If empty or "-", use standard input.
        //! Must be a regular file if @a start_offset is not zero.
        //! If @a repeat_count is not 1 and the file is not a regular one,
        //! the file is closed and reopened instead of being rewound.
        //! @param [in] repeat_count Reading packets loops back after end of
        //! file until all repeat are done. If zero, infinitely repeat.
        //! @param [in] start_offset Offset in bytes from the beginning of the file
        //! where to start reading packets at each iteration.
        //! @param [in,out] report Where to report errors.
        //! @param [in] format Expected format of the TS file.
        //! @return True on success, false on error.
        //!
        bool openRead(const UString& filename, size_t repeat_count, uint64_t start_offset, Report& report, TSPacketFormat format = TSPacketFormat::AUTODETECT);

        //!
        //! Open the file for read in rewindable mode.
        //! The file must be a rewindable file, eg. not a pipe.
        //! There is no repeat count, rewind must be done explicitly.
        //! @param [in] filename File name. If empty or "-", use standard input.
        //! @param [in] start_offset Offset in bytes from the beginning of the file
        //! where to start reading packets.
        //! @param [in,out] report Where to report errors.
        //! @param [in] format Expected format of the TS file.
        //! @return True on success, false on error.
        //! @see rewind()
        //! @see seek()
        //!
        bool openRead(const UString& filename, uint64_t start_offset, Report& report, TSPacketFormat format = TSPacketFormat::AUTODETECT);

        //!
        //! Flags for open().
        //!
        enum OpenFlags {
            NONE        = 0x0000,   //!< No option, do not open the file.
            READ        = 0x0001,   //!< Read the file.
            WRITE       = 0x0002,   //!< Write the file.
            APPEND      = 0x0004,   //!< Append packets to an existing file.
            KEEP        = 0x0008,   //!< Keep previous file with same name. Fail if it already exists.
            SHARED      = 0x0010,   //!< Write open with shared read for other processes. Windows only. Always shared on Unix.
            TEMPORARY   = 0x0020,   //!< Temporary file, deleted on close, not always visible in the file system.
            REOPEN      = 0x0040,   //!< Close and reopen the file instead of rewind to start of file when looping on input file.
            REOPEN_SPEC = 0x0080,   //!< Force REOPEN when the file is not a regular file.
        };

        //!
        //! Open or create the file (generic form).
        //! The file is rewindable if the underlying file is seekable, eg. not a pipe.
        //! @param [in] filename File name. If empty or "-", use standard input or output.
        //! If @a filename is empty, @a flags cannot contain both READ and WRITE.
        //! @param [in] flags Bit mask of open flags.
        //! @param [in,out] report Where to report errors.
        //! @param [in] format Format of the TS file.
        //! @return True on success, false on error.
        //!
        virtual bool open(const UString& filename, OpenFlags flags, Report& report, TSPacketFormat format = TSPacketFormat::AUTODETECT);

        //!
        //! Check if the file is open.
        //! @return True if the file is open.
        //!
        bool isOpen() const { return _is_open; }

        //!
        //! Get the file name.
        //! @return The file name.
        //!
        UString getFileName() const { return _filename; }

        //!
        //! Get the file name as a display string.
        //! @return The file name as a display string.
        //! Not always a valid file name. Use in error messages only.
        //!
        UString getDisplayFileName() const;

        //!
        //! Close the file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Set initial and final artificial stuffing.
        //! This method shall be called before opening the file.
        //! It specifies a number of artificial null packets which are read or written
        //! before and after the actual content of the file.
        //! @param [in] initial Number of artificial initial null packets.
        //! On read, the first @a initial read packets are null packets. The actual content
        //! of the physical file will be read afterward. On write, opening the file will
        //! immediately write  @a initial null packets, before the application has a chance
        //! to explicitly write packets.
        //! @param [in] final Number of artificial final null packets.
        //! On read, when the file is completed, after all specified repetitions, reading
        //! will successfully continue for the next @a final packets and returns null packets.
        //! On write, closing the file with automatically write @a final null packets before
        //! closing the physical file.
        //!
        void setStuffing(size_t initial, size_t final);

        //!
        //! Abort any currenly read/write operation in progress.
        //! The file is left in a broken state and can be only closed.
        //!
        void abort();

        //!
        //! Rewind the file.
        //! The file must have been opened in rewindable mode.
        //! If the file file was opened with a @a start_offset different from 0,
        //! rewinding the file means restarting at this @a start_offset.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool rewind(Report& report) { return seek(0, report); }

        //!
        //! Seek the file at a specified packet index.
        //! The file must have been opened in rewindable mode.
        //! @param [in] packet_index Seek the file to this specified packet index
        //! (plus the specified @a start_offset from open()).
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool seek(PacketCounter packet_index, Report& report);

        // Override TSPacketStream implementation
        virtual size_t readPackets(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets, Report& report) override;

    private:
        UString       _filename;         //!< Input file name.
        size_t        _repeat;           //!< Repeat count (0 means infinite)
        size_t        _counter;          //!< Current repeat count
        uint64_t      _start_offset;     //!< Initial byte offset in file
        size_t        _open_null;        //!< Number of artificial null packets to insert after open().
        size_t        _close_null;       //!< Number of artificial null packets to insert before close().
        size_t        _open_null_read;   //!< Remaining null packets to read after open().
        size_t        _close_null_read;  //!< Remaining null packets to read before close().
        volatile bool _is_open;          //!< Check if file is actually open
        OpenFlags     _flags;            //!< Flags which were specified at open
        int           _severity;         //!< Severity level for error reporting
        volatile bool _at_eof;           //!< End of file has been reached
        volatile bool _aborted;          //!< Operation has been aborted, no operation available
        bool          _rewindable;       //!< Opened in rewindable mode
        bool          _regular;          //!< Is a regular file (ie. not a pipe or special device)
        bool          _std_inout;        //!< File is standard input or output.
#if defined(TS_WINDOWS)
        ::HANDLE      _handle;           //!< File handle
#else
        int           _fd;               //!< File descriptor
#endif

        // Implementation of AbstractReadStreamInterface
        virtual bool endOfStream() override;
        virtual bool readStreamPartial(void* addr, size_t max_size, size_t& ret_size, Report& report) override;

        // Implementation of AbstractWriteStreamInterface
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, Report& report) override;

        // Read/write artificial stuffing.
        void readStuffing(TSPacket*& buffer, TSPacketMetadata*& metadata, size_t count, Report& report);
        bool writeStuffing(size_t count, Report& report);

        // Internal methods
        bool openInternal(bool reopen, Report& report);
        bool seekCheck(Report& report);
        bool seekInternal(uint64_t index, Report& report);

        // Inaccessible operations.
        TSFile& operator=(TSFile&) = delete;
        TSFile& operator=(TSFile&&) = delete;
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::TSFile::OpenFlags);
