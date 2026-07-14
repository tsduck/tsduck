//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream file input with command-line arguments.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSFile.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsDuckContext.h"
#include "tsArgs.h"

namespace ts {
    //!
    //! Transport stream file input with command-line arguments.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSFileInputArgs: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(TSFileInputArgs);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TSFileInputArgs(Report* report, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        explicit TSFileInputArgs(ReporterBase* delegate, Object* owner = nullptr);

        //!
        //! Destructor.
        //!
        virtual ~TSFileInputArgs() override;

        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //!
        void defineArgs(Args& args);

        //!
        //! Load arguments from command line.
        //! Args error indicator is set in case of incorrect arguments.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] args Command line arguments.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(DuckContext& duck, Args& args);

        //!
        //! Open the input file or files.
        //! All parameters where loaded from the command line by loadArgs().
        //! @return True on success, false on error.
        //!
        bool open();

        //!
        //! Close the input file or files.
        //! @param [in] silent If true, do not report errors. This is typically useful when the object is in some error
        //! condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        bool close(bool silent = false);

        //!
        //! Read packets.
        //! @param [out] buffer Address of the buffer for incoming packets.
        //! @param [in,out] pkt_data Array of metadata for incoming packets.
        //! @param [in] max_packets Size of @a buffer in number of packets.
        //! @return The number of actually received packets (in the range
        //! 1 to @a max_packets). Returning zero means error or end of input.
        //!
        size_t read(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets);

        //!
        //! Abort the input operation currently in progress.
        //! This method is typically invoked from another thread.
        //! The only acceptable operation after an abort() is a close().
        //!
        void abort();

    private:
        volatile bool       _aborted = true;          // Set when abortInput() is set.
        bool                _interleave = false;      // Read all files simultaneously with interleaving.
        bool                _first_terminate = false; // With _interleave, terminate when the first file terminates.
        size_t              _interleave_chunk = 0;    // Number of packets per chunk when _interleave.
        size_t              _interleave_remain = 0;   // Remaining packets to read in current chunk of current file.
        size_t              _current_filename = 0;    // Current file index in _filenames.
        size_t              _current_file = 0;        // Current file index in _files. Depends on _interleave.
        size_t              _repeat_count = 1;
        uint64_t            _start_offset = 0;
        size_t              _base_label = 0;
        TSPacketFormat      _file_format = TSPacketFormat::AUTODETECT;
        std::vector<fs::path> _filenames {};
        std::vector<size_t> _start_stuffing {};
        std::vector<size_t> _stop_stuffing {};
        std::set<size_t>    _eof {};                  // Set of file indexes having reached end of file.
        std::map<size_t,TSFile> _files {};            // Array of open files. Implemented as a map since TSFile is not copyable.

        // Get/allocate a file by index.
        TSFile& getFile(size_t file_index);

        // Open one input file.
        bool openFile(size_t name_index, size_t file_index);

        // Close all files which are currently open.
        bool closeAllFiles(bool silent);
    };
}
