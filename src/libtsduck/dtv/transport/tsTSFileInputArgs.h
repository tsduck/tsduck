//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSFileInputArgs
    {
        TS_NOCOPY(TSFileInputArgs);
    public:
        //!
        //! Default constructor.
        //!
        TSFileInputArgs() = default;

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
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool open(Report& report);

        //!
        //! Close the input file or files.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Read packets.
        //! @param [out] buffer Address of the buffer for incoming packets.
        //! @param [in,out] pkt_data Array of metadata for incoming packets.
        //! @param [in] max_packets Size of @a buffer in number of packets.
        //! @param [in,out] report Where to report errors.
        //! @return The number of actually received packets (in the range
        //! 1 to @a max_packets). Returning zero means error or end of input.
        //!
        size_t read(TSPacket* buffer, TSPacketMetadata* pkt_data, size_t max_packets, Report& report);

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
        std::vector<TSFile> _files {};                // Array of open files, only one without interleave.

        // Open one input file.
        bool openFile(size_t name_index, size_t file_index, Report& report);

        // Close all files which are currently open.
        bool closeAllFiles(Report& report);
    };
}
