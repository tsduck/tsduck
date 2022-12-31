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
        TSFileInputArgs();

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
        volatile bool       _aborted;            // Set when abortInput() is set.
        bool                _interleave;         // Read all files simultaneously with interleaving.
        bool                _first_terminate;    // With _interleave, terminate when the first file terminates.
        size_t              _interleave_chunk;   // Number of packets per chunk when _interleave.
        size_t              _interleave_remain;  // Remaining packets to read in current chunk of current file.
        size_t              _current_filename;   // Current file index in _filenames.
        size_t              _current_file;       // Current file index in _files. Depends on _interleave.
        size_t              _repeat_count;
        uint64_t            _start_offset;
        size_t              _base_label;
        TSPacketFormat      _file_format;
        UStringVector       _filenames;
        std::vector<size_t> _start_stuffing;
        std::vector<size_t> _stop_stuffing;
        std::set<size_t>    _eof;                // Set of file indexes having reached end of file.
        std::vector<TSFile> _files;              // Array of open files, only one without interleave.

        // Open one input file.
        bool openFile(size_t name_index, size_t file_index, Report& report);

        // Close all files which are currently open.
        bool closeAllFiles(Report& report);
    };
}
