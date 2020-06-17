//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  File input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsTSFile.h"

namespace ts {
    //!
    //! File input plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL FileInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(FileInputPlugin);
    public:
        //!
        //! Constructor.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //!
        FileInputPlugin(TSP* tsp);

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;
        virtual bool abortInput() override;

        //! @cond nodoxygen
        // A dummy storage value to force inclusion of this module when using the static library.
        static const int REFERENCE;
        //! @endcond

    private:
        volatile bool  _aborted;            // Set when abortInput() is set.
        bool           _interleave;         // Read all files simultaneously with interleaving.
        bool           _first_terminate;    // With _interleave, terminate when the first file terminates.
        size_t         _interleave_chunk;   // Number of packets per chunk when _interleave.
        size_t         _interleave_remain;  // Remaining packets to read in current chunk of current file.
        size_t         _current_filename;   // Current file index in _filenames.
        size_t         _current_file;       // Current file index in _files. Depends on _interleave.
        size_t         _repeat_count;
        uint64_t       _start_offset;
        size_t         _base_label;
        TSPacketFormat _file_format;
        UStringVector  _filenames;
        std::set<size_t>     _eof;          // Set of file indexes having reached end of file.
        std::vector<TSFile>  _files;        // Array of open files, only one without interleave.

        // Open one input file.
        bool openFile(size_t name_index, size_t file_index);

        // Close all files which are currently open.
        bool closeAllFiles();
    };
}
