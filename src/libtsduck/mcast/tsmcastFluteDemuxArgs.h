//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class FluteDemux.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsmcastFluteSessionId.h"

namespace ts {
    class Args;
    class DuckContext;
}

namespace ts::mcast {
    //!
    //! Command line arguments for the class FluteDemux.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL FluteDemuxArgs
    {
    public:
        //!
        //! Constructor.
        //!
        FluteDemuxArgs() = default;

        // Analysis options:
        bool           strict = false;              //!< Option -\-strict
        bool           log_flute_packets = false;   //!< Option -\-log-flute-packets
        bool           dump_flute_payload = false;  //!< Option -\-dump-flute-payload
        bool           log_fdt = false;             //!< Option -\-log-fdt
        bool           log_files = false;           //!< Option -\-log-files
        bool           dump_xml_files = false;      //!< Option -\-dump-xml-files
        fs::path       save_fdt {};                 //!< Option -\-save-fdt
        uint64_t       max_file_size = 0;           //!< Option -\-max-file-size
        UStringVector  extract_files {};            //!< Options -\-extract-file
        FluteSessionId extract_session {};          //!< Options -\-extract-source, -\-extract-destination, -\-extract-tsi
        fs::path       output_directory {};         //!< Option -\-output-directory

        //!
        //! Check if something specific was required.
        //! @return True if there is nothing to do, log or display.
        //!
        bool none() const;

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
    };
}
