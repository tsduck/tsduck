//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
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
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSFileOutputArgs
    {
        TS_NOBUILD_NOCOPY(TSFileOutputArgs);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] allow_stdout If true, the file name is optional and standard output is used by default.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        TSFileOutputArgs(Report* report, bool allow_stdout, Object* owner = nullptr);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] allow_stdout If true, the file name is optional and standard output is used by default.
        //! @param [in] owner Optional address of an "owner" object, typically an instance of class containing this object.
        //!
        TSFileOutputArgs(ReporterBase* delegate, bool allow_stdout, Object* owner = nullptr);

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
        //! @param [in] abort An optional abort interface to detect abort requests.
        //! @return True on success, false on error.
        //!
        bool open(AbortInterface* abort = nullptr);

        //!
        //! Close the output file.
        //! @param [in] silent If true, do not report errors. This is typically useful when the object is in some error
        //! condition and closing it is necessary although it may generate additional meaningless errors.
        //! @return True on success, false on error.
        //!
        bool close(bool silent = false);

        //!
        //! Write packets.
        //! @param [in] buffer Address of packets to write.
        //! @param [in] pkt_data Array of metadata for packets.
        //! A packet and its metadata have the same index in their respective arrays.
        //! @param [in] packet_count Number of packets to send from @a buffer.
        //! @param [in] abort An optional abort interface to detect abort requests.
        //! @return True on success, false on error.
        //!
        bool write(const TSPacket* buffer, const TSPacketMetadata* pkt_data, size_t packet_count, AbortInterface* abort = nullptr);

        //!
        //! Default retry interval in milliseconds.
        //!
        static constexpr cn::milliseconds DEFAULT_RETRY_INTERVAL = cn::milliseconds(2000);

    private:
        // Command line options:
        const bool        _allow_stdout;
        fs::path          _name {};
        TSFile::OpenFlags _flags = TSFile::NONE;
        TSPacketFormat    _file_format = TSPacketFormat::TS;
        bool              _reopen = false;
        cn::milliseconds  _retry_interval = DEFAULT_RETRY_INTERVAL;
        size_t            _retry_max = 0;
        size_t            _start_stuffing = 0;
        size_t            _stop_stuffing = 0;
        uint64_t          _max_size = 0;
        cn::seconds       _max_duration {0};
        size_t            _max_files = 0;
        bool              _multiple_files = false;

        // Working data:
        TSFile            _file;
        FileNameGenerator _name_gen {};
        uint64_t          _current_size = 0;
        Time              _next_open_time {};
        UStringList       _current_files {};

        // Open the file, retry on error if necessary.
        // Use max number of retries. Updated with remaining number of retries.
        bool openAndRetry(bool initial_wait, size_t& retry_allowed, AbortInterface* abort);

        // Close the current file, cleanup oldest files when necessary.
        bool closeAndCleanup(bool silent);
    };
}
