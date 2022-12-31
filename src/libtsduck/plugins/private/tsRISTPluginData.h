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
//!  Reliable Internet Stream Transport (RIST) plugins common data.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsRIST.h"
#include "tsLibRIST.h"
#include "tsPlugin.h"
#include "tsIPv4SocketAddress.h"

#if !defined(TS_NO_RIST)

namespace ts {
    //!
    //! Encapsulation of common data for RIST input and output plugins.
    //!
    class RISTPluginData
    {
        TS_NOBUILD_NOCOPY(RISTPluginData);
    public:
        //!
        //! Constructor.
        //! Also define common command line arguments for RIST plugins.
        //! @param [in,out] args Arguments to define.
        //! @param [in,out] tsp TSP plugin handler.
        //!
        RISTPluginData(Args* args, TSP* tsp);

        //!
        //! Destructor.
        //!
        ~RISTPluginData() { cleanup(); }

        //!
        //! Get command line options.
        //! @param [in,out] args Arguments to load.
        //! @return True on success, false on error.
        //!
        bool getOptions(Args* args);

        //!
        //! Add all URL's as peers in the RIST context.
        //! @return True on success, false on error.
        //!
        bool addPeers();

        //!
        //! Cleanup RIST context.
        //!
        void cleanup();

        // Working data.
        ::rist_profile          profile;  //!< RIST profile.
        ::rist_ctx*             ctx;      //!< RIST context.
        ::rist_logging_settings log;      //!< RIST logging settings.

    private:
        // Working data.
        TSP*     _tsp;
        uint32_t _buffer_size;
        int      _encryption_type;
        UString  _secret;
        int      _stats_interval;
        UString  _stats_prefix;
        IPv4SocketAddressVector          _allowed;
        IPv4SocketAddressVector          _denied;
        UStringVector                    _peer_urls;
        std::vector<::rist_peer_config*> _peer_configs;

        // Analyze a list of options containing socket addresses.
        bool getSocketValues(Args* args, IPv4SocketAddressVector& list, const UChar* option);

        // Convert between RIST log level and TSDuck severity.
        static int RistLogToSeverity(::rist_log_level level);
        static ::rist_log_level SeverityToRistLog(int severity);

        // A RIST log callback using a RISTPluginData* argument.
        static int LogCallback(void* arg, ::rist_log_level level, const char* msg);

        // A RIST stats callback using a RISTPluginData* argument.
        static int StatsCallback(void* arg, const ::rist_stats* stats);

        // A RIST connection/disconnection callback using a RISTPluginData* argument.
        static int ConnectCallback(void* arg, const char* ip, uint16_t port, const char* local_ip, uint16_t local_port, ::rist_peer* peer);
        static int DisconnectCallback(void* arg, ::rist_peer* peer);
    };
}

#endif // TS_NO_RIST
