//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        //! @param [in,out] report Where to report errors.
        //!
        RISTPluginData(Report& report);

        //!
        //! Destructor.
        //!
        ~RISTPluginData() { cleanup(); }

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
        //! Add all URL's as peers in the RIST context.
        //! @return True on success, false on error.
        //!
        bool addPeers();

        //!
        //! Cleanup RIST context.
        //!
        void cleanup();

        // Working data.
        ::rist_profile          profile = RIST_PROFILE_SIMPLE; //!< RIST profile.
        ::rist_ctx*             ctx = nullptr;                 //!< RIST context.
        ::rist_logging_settings log {};                        //!< RIST logging settings.

    private:
        // Working data.
        Report&  _report;
        uint32_t _buffer_size = 0;
        int      _encryption_type = 0;
        UString  _secret {};
        int      _stats_interval = 0;
        UString  _stats_prefix {};
        IPv4SocketAddressVector          _allowed {};
        IPv4SocketAddressVector          _denied {};
        UStringVector                    _peer_urls {};
        std::vector<::rist_peer_config*> _peer_configs {};

        // Analyze a list of options containing socket addresses.
        bool getSocketValues(Args& args, IPv4SocketAddressVector& list, const UChar* option);

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
