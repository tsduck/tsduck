//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class ECMGClient.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsIPv4SocketAddress.h"
#include "tstlv.h"

namespace ts {

    class Args;
    class DuckContext;

    //!
    //! Command line arguments for the class ECMGClient.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL ECMGClientArgs
    {
    public:
        //!
        //! Constructor.
        //!
        ECMGClientArgs() = default;

        // Public fields, by options.
        IPv4SocketAddress ecmg_address {};      //!< -\-ecmg, ECMG socket address (required or optional)
        uint32_t          super_cas_id = 0;     //!< -\-super-cas-id, CA system & subsystem id
        ByteBlock         access_criteria {};   //!< -\-access-criteria
        MilliSecond       cp_duration = 0;      //!< -\-cp-duration, crypto-period duration
        tlv::VERSION      dvbsim_version = 0;   //!< -\-ecmg-scs-version
        uint16_t          ecm_channel_id = 0;   //!< -\-channel-id
        uint16_t          ecm_stream_id = 0;    //!< -\-stream-id
        uint16_t          ecm_id = 0;           //!< -\-ecm-id
        int               log_protocol = 0;     //!< -\-log-protocol
        int               log_data = 0;         //!< -\-log-data

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
