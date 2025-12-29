//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for transport stream packets dump.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"

namespace ts {

    class Args;
    class DuckContext;
    class TSPacket;
    class TSPacketMetadata;

    //!
    //! Command line arguments for transport stream packets dump.
    //! @ingroup libtsduck cmd
    //!
    class TSDUCKDLL TSDumpArgs
    {
        TS_NOCOPY(TSDumpArgs);
    public:
        // Public fields
        uint32_t dump_flags = 0;  //!< Dump options for Hexa and Packet::dump
        bool     rs204 = false;   //!< Option -\-rs204
        bool     log = false;     //!< Option -\-log
        size_t   log_size = 0;    //!< Size to display with -\-log
        PIDSet   pids {};         //!< PID values to dump

        //!
        //! Default constructor.
        //!
        TSDumpArgs() = default;

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
        //! This method displays the content of a transport packet according to the command line options.
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in,out] strm A standard stream in output mode (text mode).
        //! @param [in] pkt TS packet to dump.
        //! @param [in] mdata Optional metadata.
        //!
        void dump(DuckContext& duck, std::ostream& strm, const TSPacket& pkt, const TSPacketMetadata* mdata = nullptr) const;
    };
}
