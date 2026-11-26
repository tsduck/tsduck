//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream processor: internal core API.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSProcessorCommonArgs.h"
#include "tsReport.h"
#include "tsTS.h"

namespace ts::tsp {
    //!
    //! Transport stream processor internal core API.
    //! This class exposes the TSProcessor API that the various plugin threads can use.
    //! There is one instance of this class per TSProcessor.
    //! This class is internal to the TSDuck library and cannot be called by applications.
    //! @ingroup libtsduck plugin
    //!
    class Core
    {
        TS_NOBUILD_NOCOPY(Core);
    public:
        //!
        //! Constructor.
        //! @param [in] options Common command line options for tsp.
        //! @param [in,out] report Where to report errors, logs, etc.
        //!
        Core(const TSProcessorCommonArgs& options, Report& report);

        //!
        //! Provide access to the global message report of the transport stream processor.
        //! @return A reference to the global message report.
        //!
        Report& report() { return _report; }

        //!
        //! Provide access to the common command line options of the transport stream processor.
        //! @return A reference to the common command line options.
        //!
        const TSProcessorCommonArgs& commonArgs() const { return _args; }

        //!
        //! Reset this object, typically when the TSProcessor starts.
        //!
        void reset();

        //!
        //! Provide access to the global mutex of the transport stream processor.
        //! It can be used to serialize global operations inside the transport stream processor.
        //! It shall not be held too long.
        //! @return A reference to the global mutex of the transport stream processor.
        //!
        std::recursive_mutex& globalMutex() { return _global_mutex; }

        //!
        //! Declare that a plugin uses (@a on = true) or no longer uses (@a on = false) joint termination.
        //! @param [in] on True to declare one more user, false to declare one less.
        //! @return The new number of joint termination users.
        //!
        int declareJointTerminationUser(bool on);

        //!
        //! Declare that a plugin performs joint termination.
        //! @param [in] total_packets_in_thread The total number of packets which were processed in the
        //! plugin thread. This also includes passed which were not passed to the plugin class.
        //! @return The remaining number of non-terminated joint termination users.
        //!
        int declareJointTermination(PacketCounter total_packets_in_thread);

        //!
        //! Get the packet number after which the "joint termination" must be applied.
        //! @return The packet number after which the "joint termination" must be applied.
        //! If no "joint termination" applies, return the maximum int value.
        //!
        PacketCounter totalPacketsBeforeJointTermination();

    private:
        const TSProcessorCommonArgs& _args;
        Report&                      _report;
        std::recursive_mutex         _global_mutex {};     // Global mutex of the transport stream processor.
        int                          _jt_users = 0;        // Number of plugins using "joint termination".
        int                          _jt_remaining = 0;    // Number of plugins using jt but not yet completed.
        PacketCounter                _jt_highest_pkt = 0;  // Highest packet number for completed jt plugins.
    };
}
