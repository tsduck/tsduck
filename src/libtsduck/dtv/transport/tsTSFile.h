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
#include "tsBinaryFile.h"
#include "tsTSPacketStream.h"

namespace ts {
    //!
    //! Transport stream file, input and/or output.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSFile: public BinaryFile, public TSPacketStream
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
        //! Open or create the file (generic form).
        //! The file is rewindable if the underlying file is seekable, eg. not a pipe.
        //! @param [in] filename File name. If empty or "-", use standard input or output.
        //! If @a filename is empty, @a flags cannot contain both READ and WRITE.
        //! @param [in] flags Bit mask of open flags.
        //! @param [in] format Format of the TS file.
        //! @return True on success, false on error.
        //!
        virtual bool open(const fs::path& filename, OpenFlags flags, TSPacketFormat format);

        // Inherited version from BinaryFile, format is AUTODETECT.
        virtual bool open(const fs::path& filename, OpenFlags flags) override;

        //!
        //! Close the file.
        //! @param [in] silent If true, do not report errors. This is typically useful when the object is in some error
        //! condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        virtual bool close(bool silent = false) override;

        //!
        //! Set initial and final artificial stuffing.
        //! This method shall be called before opening the file.
        //! It specifies a number of artificial null packets which are read or written
        //! before and after the actual content of the file.
        //! @param [in] initial Number of artificial initial null packets.
        //! On read, the first @a initial read packets are null packets. The actual content
        //! of the physical file will be read afterward. On write, opening the file will
        //! immediately write @a initial null packets, before the application has a chance
        //! to explicitly write packets.
        //! @param [in] final Number of artificial final null packets.
        //! On read, when the file is completed, after all specified repetitions, reading
        //! will successfully continue for the next @a final packets and returns null packets.
        //! On write, closing the file with automatically write @a final null packets before
        //! closing the physical file.
        //!
        void setStuffing(size_t initial, size_t final);

        //!
        //! Seek the file at a specified packet index.
        //! The file must have been opened in rewindable mode.
        //! @param [in] packet_index Seek the file to this specified packet index (plus the specified @a start_offset from open()).
        //! @return True on success, false on error.
        //!
        bool seekPacket(PacketCounter packet_index);

        // Implementation of AbstractStream
        virtual bool endOfStream() override;

        // Override TSPacketStream implementation
        virtual size_t readPackets(TSPacket* buffer, TSPacketMetadata* metadata, size_t max_packets) override;

    private:
        size_t _open_null = 0;        // Number of artificial null packets to insert after open().
        size_t _close_null = 0;       // Number of artificial null packets to insert before close().
        size_t _open_null_read = 0;   // Remaining null packets to read after open().
        size_t _close_null_read = 0;  // Remaining null packets to read before close().

        // Initialize TS-specific state over open.
        bool wrapOpen(bool open_status, TSPacketFormat format);

        // Read/write artificial stuffing.
        void readStuffing(TSPacket*& buffer, TSPacketMetadata*& metadata, size_t count);
        bool writeStuffing(size_t count);
    };
}
