//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Continuity counters analysis and repair.
//!
//----------------------------------------------------------------------------

#pragma once
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
        //! @param [in] pid_filter The set of PID's to analyze or fix.
        //! @param [in] report Where to report discontinuity errors. Drop errors if null.
        //!
        explicit ContinuityAnalyzer(const PIDSet& pid_filter = NoPID, Report* report = nullptr);

        // Implementation note:
        // "= default" definitions required for copy constructor and assignments so that
        // the compiler understands that we know what we do with the pointer member _report.
        // Important: take care in case of internal modification, do not break the ownership
        // of pointers because the compiler will no longer complain.

        //!
        //! Copy constructor.
        //! @param [in] other Other instance to copy.
        //!
        ContinuityAnalyzer(const ContinuityAnalyzer& other) = default;

        //!
        //! Assignment operator.
        //! @param [in] other Other instance to copy.
        //! @return A reference to this instance.
        //!
        ContinuityAnalyzer& operator=(const ContinuityAnalyzer& other) = default;

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
        bool feedPacket(const TSPacket& pkt) { return feedPacketInternal(const_cast<TSPacket*>(&pkt), false); }

        //!
        //! Process or modify a TS packet.
        //! @param [in,out] pkt A transport stream packet.
        //! It can be modified only when error fixing or generator mode is activated.
        //! @return True if the packet had no discontinuity error and is unmodified.
        //! False if the packet had an error or was modified.
        //!
        bool feedPacket(TSPacket& pkt) { return feedPacketInternal(&pkt, true); }

        //!
        //! Get the total number of TS packets.
        //! @return The total number of TS packets.
        //!
        PacketCounter totalPackets() const { return _total_packets; }

        //!
        //! Get the number of processed TS packets.
        //! Only packets from selected PID's are counted.
        //! @return The number of processed TS packets.
        //!
        PacketCounter processedPackets() const { return _processed_packets; }

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
        //! When fixing errors, specify how to handle duplicated packets.
        //! Two successive packets in the same PID are considered as duplicated if
        //! they have the same continuity counter and same content (except PCR, if any).
        //! @param [in] on When true (the default), duplicated input packets are replicated
        //! as duplicated on output (the corresponding output packets have the same continuity
        //! counters). When false, the input packets are not considered as duplicated and the
        //! output packets have incremented countinuity counters.
        //!
        void setReplicateDuplicated(bool on) { _replicate_dup = on; }

        //!
        //! Set generator mode.
        //! When the generator mode is on, the input continuity counters are always ignored.
        //! The output continuity counters are updated to create a continuous stream.
        //! No error is reported.
        //! @param [in] gen When true, activate generator mode.
        //!
        void setGenerator(bool gen) { _generator = gen; }

        //!
        //! Define the severity of messages.
        //! The default severity is Severity::Info.
        //! @param [in] level The severity of each message.
        //!
        void setMessageSeverity(int level) { _severity = level; }

        //!
        //! Define a prefix string to be displayed with each message.
        //! @param [in] prefix The prefix string to be displayed with each message.
        //!
        void setMessagePrefix(const UString& prefix) { _prefix = prefix; }

        //!
        //! Specify to log messages in JSON format.
        //! If a message prefix is set, it is logged just before the JSON structure
        //! and can be used to locate the appropriate JSON messages in a flow of logs.
        //! @param [in] on Set the JSON mode on or off.
        //!
        void setJSON(bool on) { _json = on; }

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
        size_t pidCount() const { return _pid_filter.count(); }

        //!
        //! Check if a PID is processed.
        //! @param [in] pid The PID to test.
        //! @return True if @a pid is processed.
        //!
        bool hasPID(PID pid) const;

        //!
        //! Get the first CC in a PID.
        //! @param [in] pid The PID to check.
        //! @return The first CC value in the PID or ts::INVALID_CC when the PID is not filtered.
        //! The first CC in a PID is never modified.
        //!
        uint8_t firstCC(PID pid) const;

        //!
        //! Get the last CC in a PID.
        //! @param [in] pid The PID to check.
        //! @return The last CC value in the PID or ts::INVALID_CC when the PID is not filtered.
        //! This is the output CC value, possibly modified.
        //!
        uint8_t lastCC(PID pid) const;

        //!
        //! Get the last duplicate packet count for a PID.
        //! @param [in] pid The PID to check.
        //! @return The last duplicate packet count for the PID or ts::NPOS when the PID is not filtered.
        //!
        size_t dupCount(PID pid) const;

        //!
        //! Get the last transport stream packet that was passed to feedPacket() for a PID.
        //! @param [in] pid The PID to check.
        //! @return The last packet for the PID or ts::NullPacket when the PID is not filtered.
        //!
        TSPacket lastPacket(PID pid) const;

        //!
        //! Get the last transport stream packet that was passed to feedPacket() for a PID.
        //! @param [in] pid The PID to check.
        //! @param [out] packet The last packet for the PID or ts::NullPacket when the PID is not filtered.
        //!
        void getLastPacket(PID pid, TSPacket& packet) const;

        //!
        //! Compute the number of missing packets between two continuity counters.
        //! @param [in] cc1 First continuity counter.
        //! @param [in] cc2 Second continuity counter.
        //! @return Number of missing packets between @a cc1 and @a cc2.
        //!
        static size_t MissingPackets(int cc1, int cc2);

    private:
        // PID analysis state
        class PIDState
        {
        public:
            PIDState() = default;              // Constructor
            uint8_t  first_cc = INVALID_CC;    // First CC value in a PID.
            uint8_t  last_cc_out = INVALID_CC; // Last output CC value in a PID.
            size_t   dup_count = 0;            // Consecutive duplicate count.
            TSPacket last_pkt_in {};           // Last input packet (before modification, if any).
        };

        // A map of PID state, indexed by PID.
        typedef std::map<PID,PIDState> PIDStateMap;

        // Private members.
        Report*       _report;                    // Where to report errors, never null.
        int           _severity = Severity::Info; // Severity level for error messages.
        bool          _display_errors = false;    // Display discontinuity errors.
        bool          _fix_errors = false;        // Fix discontinuity errors.
        bool          _replicate_dup = true;      // With _fix_errors, replicate duplicate packets.
        bool          _generator = false;         // Use generator mode.
        bool          _json = false;              // Log JSON messages.
        UString       _prefix {};                 // Message prefix.
        PacketCounter _total_packets = 0;         // Total number of packets.
        PacketCounter _processed_packets = 0;     // Number of processed packets.
        PacketCounter _fix_count = 0;             // Number of fixed (modified) packets.
        PacketCounter _error_count = 0;           // Number of discontinuity errors.
        PIDSet        _pid_filter {};             // Current set of filtered PID's.
        PIDStateMap   _pid_states {};             // State of all PID's.

        // Internal version of feedPacket.
        // The packet is modified only if update is true.
        bool feedPacketInternal(TSPacket* pkt, bool update);

        // Build the first part of an error message.
        UString linePrefix(PID pid) const;

        // Log a JSON message.
        void logJSON(PID pid, const UChar* type, size_t packet_count = NPOS);
    };
}
