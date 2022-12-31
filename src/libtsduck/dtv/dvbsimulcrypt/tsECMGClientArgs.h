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
        ECMGClientArgs();

        // Public fields, by options.
        IPv4SocketAddress ecmg_address;     //!< -\-ecmg, ECMG socket address (required or optional)
        uint32_t          super_cas_id;     //!< -\-super-cas-id, CA system & subsystem id
        ByteBlock         access_criteria;  //!< -\-access-criteria
        MilliSecond       cp_duration;      //!< -\-cp-duration, crypto-period duration
        tlv::VERSION      dvbsim_version;   //!< -\-ecmg-scs-version
        uint16_t          ecm_channel_id;   //!< -\-channel-id
        uint16_t          ecm_stream_id;    //!< -\-stream-id
        uint16_t          ecm_id;           //!< -\-ecm-id
        int               log_protocol;     //!< -\-log-protocol
        int               log_data;         //!< -\-log-data

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
