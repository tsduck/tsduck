//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsTSPacketStream.h"
#include "tsAbstractStream.h"
#include "tsEnumUtils.h"

namespace ts {

    class TSPacketMetadata;

    //!
    //! Transport stream file, input and/or output.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSFile: public ReporterBase, public TSPacketStream, private AbstractStream
    {
        TS_NOBUILD_NOCOPY(TSFile);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TSFile(Report* report, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TSFile(ReporterBase* delegate, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~TSFile() override;

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
        //! @param [in] format Expected format of the TS file.
        //! @return True on success, false on error.
        //!
        bool openRead(const fs::path& filename, size_t repeat_count, uint64_t start_offset, TSPacketFormat format = TSPacketFormat::AUTODETECT);

        //!
        //! Open the file for read in rewindable mode.
        //! The file must be a rewindable file, eg. not a pipe.
        //! There is no repeat count, rewind must be done explicitly.
        //! @param [in] filename File name. If empty or "-", use standard input.
        //! @param [in] start_offset Offset in bytes from the beginning of the file
        //! where to start reading packets.
        //! @param [in] format Expected format of the TS file.
        //! @return True on success, false on error.
        //! @see rewind()
        //! @see seek()
        //!
        bool openRead(const fs::path& filename, uint64_t start_offset, TSPacketFormat format = TSPacketFormat::AUTODETECT);

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
        //! @param [in] format Format of the TS file.
        //! @return True on success, false on error.
        //!
        virtual bool open(const fs::path& filename, OpenFlags flags, TSPacketFormat format = TSPacketFormat::AUTODETECT);

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
        bool close(bool silent = false);

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
        //! @return True on success, false on error.
        //!
        bool rewind() { return seekPacket(0); }

        //!
        //! Seek the file at a specified packet index.
        //! The file must have been opened in rewindable mode.
        //! @param [in] packet_index Seek the file to this specified packet index (plus the specified @a start_offset from open()).
        //! @return True on success, false on error.
        //!
        bool seekPacket(PacketCounter packet_index);

        //!
        //! Seek the file at a specified byte index.
        //! The file must have been opened in rewindable mode.
        //! @param [in] byte_index Seek the file to this specified byte index (plus the specified @a start_offset from open()).
        //! @return True on success, false on error.
        //!
        bool seekByte(uint64_t byte_index);

        // Override TSPacketStream implementation
        virtual size_t readPackets(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets) override;

    private:
        fs::path      _filename {};          //!< Input file name.
        size_t        _repeat = 0;           //!< Repeat count (0 means infinite)
        size_t        _counter = 0;          //!< Current repeat count
        uint64_t      _start_offset = 0;     //!< Initial byte offset in file
        size_t        _open_null = 0;        //!< Number of artificial null packets to insert after open().
        size_t        _close_null = 0;       //!< Number of artificial null packets to insert before close().
        size_t        _open_null_read = 0;   //!< Remaining null packets to read after open().
        size_t        _close_null_read = 0;  //!< Remaining null packets to read before close().
        volatile bool _is_open = false;      //!< Check if file is actually open
        OpenFlags     _flags = NONE;         //!< Flags which were specified at open
        int           _severity = Severity::Error;   //!< Severity level for error reporting
        volatile bool _at_eof = false;       //!< End of file has been reached
        volatile bool _aborted = false;      //!< Operation has been aborted, no operation available
        bool          _rewindable = false;   //!< Opened in rewindable mode
        bool          _regular = false;      //!< Is a regular file (ie. not a pipe or special device)
        bool          _std_inout = false;    //!< File is standard input or output.
#if defined(TS_WINDOWS)
        ::HANDLE      _handle = nullptr;
#else
        int           _fd = -1;
#endif

        // Implementation of AbstractStream
        virtual bool endOfStream() override;
        virtual bool readStream(void* addr, size_t max_size, size_t& ret_size) override;
        virtual bool writeStream(const void* addr, size_t size, size_t& written_size) override;

        // Read/write artificial stuffing.
        void readStuffing(TSPacket*& buffer, TSPacketMetadata*& metadata, size_t count);
        bool writeStuffing(size_t count);

        // Internal methods
        bool openInternal(bool reopen);
        bool seekCheck();
    };
}

TS_ENABLE_BITMASK_OPERATORS(ts::TSFile::OpenFlags);
