//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream file output with command-line arguments.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSFile.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsFileNameGenerator.h"
#include "tsDuckContext.h"
#include "tsAbortInterface.h"
#include "tsArgs.h"

namespace ts {
    //!
    //! Transport stream file output with command-line arguments.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSFileOutputArgs
    {
        TS_NOBUILD_NOCOPY(TSFileOutputArgs);
    public:
        //!
        //! Default constructor.
        //! @param [in] allow_stdout If true, the file name is optional and standard output is used by default.
        //!
        TSFileOutputArgs(bool allow_stdout);

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
        //! Open the output file.
        //! All parameters where loaded from the command line by loadArgs().
        //! @param [in,out] report Where to report errors.
        //! @param [in] abort An optional abort interface to detect abort requests.
        //! @return True on success, false on error.
        //!
        bool open(Report& report, AbortInterface* abort = nullptr);

        //!
        //! Close the output file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool close(Report& report);

        //!
        //! Write packets.
        //! @param [in] buffer Address of packets to write.
        //! @param [in] pkt_data Array of metadata for packets.
        //! A packet and its metadata have the same index in their respective arrays.
        //! @param [in] packet_count Number of packets to send from @a buffer.
        //! @param [in,out] report Where to report errors.
        //! @param [in] abort An optional abort interface to detect abort requests.
        //! @return True on success, false on error.
        //!
        bool write(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count, Report& report, AbortInterface* abort = nullptr);

        //!
        //! Default retry interval in milliseconds.
        //!
        static constexpr MilliSecond DEFAULT_RETRY_INTERVAL  = 2000;

    private:
        // Command line options:
        const bool        _allow_stdout;
        fs::path          _name {};
        TSFile::OpenFlags _flags = TSFile::NONE;
        TSPacketFormat    _file_format = TSPacketFormat::TS;
        bool              _reopen = false;
        MilliSecond       _retry_interval = DEFAULT_RETRY_INTERVAL;
        size_t            _retry_max = 0;
        size_t            _start_stuffing = 0;
        size_t            _stop_stuffing = 0;
        uint64_t          _max_size = 0;
        Second            _max_duration = 0;
        size_t            _max_files = 0;
        bool              _multiple_files = false;

        // Working data:
        TSFile            _file {};
        FileNameGenerator _name_gen {};
        uint64_t          _current_size = 0;
        Time              _next_open_time {};
        UStringList       _current_files {};

        // Open the file, retry on error if necessary.
        // Use max number of retries. Updated with remaining number of retries.
        bool openAndRetry(bool initial_wait, size_t& retry_allowed, Report& report, AbortInterface* abort);

        // Close the current file, cleanup oldest files when necessary.
        bool closeAndCleanup(Report& report);
    };
}
