//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  Continuity counters analysis and repair.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsTSPacket.h"
#include "tsReport.h"

namespace ts {
    //!
    //! Continuity counters analysis and repair.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL ContinuityAnalyzer
    {
    public:
        //!
        //! Constructor.
        //! @param [in] display When true, display discontinuity errors on @a report.
        //! @param [in] fix When true, fix discontinuity errors.
        //! @param [in] pid_filter The set of PID's to analyze or fix.
        //! @param [in] report Where to report discontinuity errors. Drop errors if null.
        //!
        explicit ContinuityAnalyzer(bool display = false, bool fix = false, const PIDSet& pid_filter = NoPID, Report* report = nullptr);

        //!
        //! Destructor.
        //!
        ~ContinuityAnalyzer();

        //!
        //! Reset all collected information.
        //! Do not change processing options (display and/or fix errors).
        //!
        void reset();

        //!
        //! Process a constant TS packet.
        //! Can be used only to report discontinuity errors.
        //! @param [in] pkt A transport stream packet.
        //! @return True if the packet has no discontinuity error. False if it has an error.
        //!
        bool feedPacket(const TSPacket& pkt);

        //!
        //! Process or modify a TS packet.
        //! @param [in,out] pkt A transport stream packet. It can be modified only when error fixing is activated.
        //! @return True if the packet had no discontinuity error. False if it had an error.
        //! If fixing is activated, the error is fixed and false is returned.
        //!
        bool feedPacket(TSPacket& pkt);

        //!
        //! Get the number of processed TS packets.
        //! @return The number of processed TS packets.
        //!
        PacketCounter packetCount() const { return _packet_count; }

        //!
        //! Get the number of fixed (modified) TS packets.
        //! @return The number of fixed (modified) TS packets.
        //!
        PacketCounter fixCount() const { return _fix_count; }

        //!
        //! Get the number of discontinuity errors.
        //! @return The number of discontinuity errors.
        //!
        PacketCounter errorCount() const { return _error_count; }

        //!
        //! Change the output device to report errors.
        //! @param [in] report Where to report discontinuity errors. Drop errors if null.
        //!
        void setReport(Report* report);

        //!
        //! Change error reporting.
        //! @param [in] display When true, display discontinuity errors.
        //!
        void setDisplay(bool display) { _display_errors = display; }

        //!
        //! Change error fixing.
        //! @param [in] fix When true, fix discontinuity errors.
        //!
        void setFix(bool fix) { _fix_errors = fix; }

        //!
        //! Replace the list of PID's to process.
        //! @param [in] pid_filter The list of PID's to process.
        //!
        void setPIDFilter(const PIDSet& pid_filter);

        //!
        //! Add one PID to process.
        //! @param [in] pid The new PID to process.
        //!
        void addPID(PID pid);

        //!
        //! Add several PID's to process.
        //! @param [in] pids The list of new PID's to process.
        //!
        void addPIDs(const PIDSet& pids);

        //!
        //! Remove one PID to process.
        //! @param [in] pid The PID to no longer process.
        //!
        void removePID(PID pid);

        //!
        //! Get the current number of PID's being processed.
        //! @return The current number of PID's being processed.
        //!
        size_t pidCount() const;

        //!
        //! Check if a PID is processed.
        //! @param [in] pid The PID to test.
        //! @return Tue if @a pid is processed.
        //!
        bool hasPID(PID pid) const;

    private:
        Report*       _report;          // Where to report errors, never null.
        bool          _display_errors;  // Display discontinuity errors.
        bool          _fix_errors;      // Fix discontinuity errors.
        PacketCounter _packet_count;    // Number of processed packets.
        PacketCounter _fix_count;       // Number of fixed (modified) packets.
        PacketCounter _error_count;     // Number of discontinuity errors.
        PIDSet        _pid_filter;      // Current set of filtered PID's.

        // Reset the context for one single PID.
        void resetPID(PID pid);

        // Unreachable constructors and operators.
        ContinuityAnalyzer(const ContinuityAnalyzer&) = delete;
        ContinuityAnalyzer& operator=(const ContinuityAnalyzer&) = delete;
    };
}
