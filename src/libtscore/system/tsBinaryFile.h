//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Binary stream file, with optional asynchronous I/O.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNonBlockingDevice.h"
#include "tsStreamInterface.h"
#include "tsEnumUtils.h"

namespace ts {
    //!
    //! Binary stream file, with optional asynchronous I/O.
    //! @ingroup libtsduck mpeg
    //!
    class TSCOREDLL BinaryFile: public NonBlockingDevice, public StreamInterface
    {
        TS_NOBUILD_NOCOPY(BinaryFile);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] non_blocking It true, the file is initially set in non-blocking mode.
        //!
        explicit BinaryFile(Report* report, bool non_blocking = false);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] non_blocking It true, the file is initially set in non-blocking mode.
        //!
        explicit BinaryFile(ReporterBase* delegate, bool non_blocking = false);

        //!
        //! Destructor.
        //!
        virtual ~BinaryFile() override;

        //!
        //! Open the file for read with a given repetition count.
        //! The file is automatically read a given number of times before declaring an end-of-file.
        //! No rewind or seek operation is allowed because it would break the repetition.
        //! @param [in] filename File name. If empty or "-", use standard input.
        //! Must be a regular file if @a start_offset is not zero.
        //! If @a repeat_count is not 1 and the file is not a regular one,
        //! the file is closed and reopened instead of being rewound.
        //! @param [in] repeat_count Reading the file loops back after end of
        //! file until all repeat are done. If zero, infinitely repeat.
        //! @param [in] start_offset Offset in bytes from the beginning of the file
        //! where to start reading data at each iteration.
        //! @return True on success, false on error.
        //!
        bool openRead(const fs::path& filename, size_t repeat_count, uint64_t start_offset);

        //!
        //! Open the file for read in rewindable mode.
        //! The file must be a rewindable file, eg. not a pipe.
        //! There is no repeat count, rewind must be done explicitly.
        //! @param [in] filename File name. If empty or "-", use standard input.
        //! @param [in] start_offset Offset in bytes from the beginning of the file where to start reading data.
        //! @return True on success, false on error.
        //! @see rewind()
        //! @see seekByte()
        //!
        bool openRead(const fs::path& filename, uint64_t start_offset);

        //!
        //! Flags for open().
        //!
        enum OpenFlags {
            NONE        = 0x0000,   //!< No option, do not open the file.
            READ        = 0x0001,   //!< Read the file.
            WRITE       = 0x0002,   //!< Write the file.
            APPEND      = 0x0004,   //!< Append data to an existing file.
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
        //! @return True on success, false on error.
        //!
        virtual bool open(const fs::path& filename, OpenFlags flags);

        //!
        //! Get the flags which were used when the file was open.
        //! @return The flags.
        //!
        OpenFlags getFlags() const { return _flags; }

        //!
        //! Check if the file is open.
        //! @return True if the file is open.
        //!
        bool isOpen() const { return _is_open; }

        //!
        //! Get the file name.
        //! @return The file name.
        //!
        fs::path getFileName() const { return _filename; }

        //!
        //! Get the file name as a display string.
        //! @return The file name as a display string.
        //! Not always a valid file name. Use in error messages only.
        //!
        UString getDisplayFileName() const;

        //!
        //! Close the file.
        //! @param [in] silent If true, do not report errors. This is typically useful when the object is in some error
        //! condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        virtual bool close(bool silent = false);

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
        //! @return True on success, false on error.
        //!
        bool rewind() { return seekByte(0); }

        //!
        //! Seek the file at a specified byte index.
        //! The file must have been opened in rewindable mode.
        //! @param [in] byte_index Seek the file to this specified byte index (plus the specified @a start_offset from open()).
        //! @return True on success, false on error.
        //!
        bool seekByte(uint64_t byte_index);

        // Implementation of NonBlockingDevice.
        virtual SysHandleType getHandle() const override;
        virtual SysSocketType getSocket() const override;

        // Implementation of StreamInterface.
        virtual bool readStream(void* addr, size_t size, const AbortInterface* abort = nullptr) override;
        virtual bool readStream(void* addr, size_t max_size, size_t& ret_size, const AbortInterface* abort = nullptr, IOSB* iosb = nullptr) override;
        virtual bool writeStream(const void* addr, size_t size, IOSB* iosb = nullptr) override;
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size, IOSB* iosb = nullptr) override;
        virtual bool asyncCompletedStream(IOSB* iosb) override;
        virtual bool isReadStream() override;
        virtual bool isWriteStream() override;
        virtual bool endOfStream() override;

    protected:
        // Inherited methods.
        virtual bool allowSetNonBlocking() const override;

    private:
        fs::path      _filename {};          // Input file name.
        size_t        _repeat_input = 0;     // Repeat count (0 means infinite)
        size_t        _input_counter = 0;    // Current repeat count
        uint64_t      _start_offset = 0;     // Initial byte offset in file
        volatile bool _is_open = false;      // Check if file is actually open
        OpenFlags     _flags = NONE;         // Flags which were specified at open
        volatile bool _at_eof = false;       // End of file has been reached
        volatile bool _aborted = false;      // Operation has been aborted, no operation available
        bool          _rewindable = false;   // Opened in rewindable mode
        bool          _regular = false;      // Is a regular file (ie. not a pipe or special device)
        bool          _std_inout = false;    // File is standard input or output.
        SysHandleType _hfd = SYS_HANDLE_INVALID;  // File handle / file descriptor.
        uint64_t      _position = 0;         // Current position in the file. Must be maintained for asynchronous I/O on Windows.

        // Common code.
        bool openInternal(bool reopen);
        bool seekByteInternal(uint64_t byte_index);

        // Check if seek is possible. Called during open to see if we can reach the start point.
        bool initialSeekCheck();

#if defined(TS_WINDOWS)
        // For Windows asynchronous I/O, we need to keep parameters in one single structure which lives during the I/O.
        // This is required because Windows does not maintain the file position in case of asynchronous I/O.
        class TSCOREDLL FileAsyncBuffers: public Object
        {
            TS_NOBUILD_NOCOPY(FileAsyncBuffers);
        public:
            bool     write_op;      // Write vs. read operation.
            size_t   req_size;      // Original I/O request size.
            uint64_t req_position;  // Current position in the file at start of I/O.

            // Constructor and destructor.
            FileAsyncBuffers(bool wrt, size_t size, uint64_t pos) : write_op(wrt), req_size(size), req_position(pos) {}
            virtual ~FileAsyncBuffers() override;
        };
#endif
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::BinaryFile::OpenFlags);
