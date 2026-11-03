//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Continuity counters analysis and repair.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDemux.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Continuity counters analysis and repair.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL ContinuityAnalyzer: public AbstractDemux
    {
        TS_NOBUILD_NOCOPY(ContinuityAnalyzer);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        using SuperClass = AbstractDemux;

        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] pid_filter The set of PID's to analyze or fix.
        //!
        explicit ContinuityAnalyzer(DuckContext& duck, const PIDSet& pid_filter = NoPID());

        //!
        //! Process a constant TS packet.
        //! Can be used only to report discontinuity errors.
        //! @param [in] pkt A transport stream packet.
        //! @return True if the packet has no discontinuity error. False if it has an error.
        //!
        bool processPacket(const TSPacket& pkt) { return processPacketInternal(const_cast<TSPacket*>(&pkt), false); }

        //!
        //! Process or modify a TS packet.
        //! @param [in,out] pkt A transport stream packet.
        //! It can be modified only when error fixing or generator mode is activated.
        //! @return True if the packet had no discontinuity error and is unmodified.
        //! False if the packet had an error or was modified.
        //!
        bool processPacket(TSPacket& pkt) { return processPacketInternal(&pkt, true); }

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
        //! output packets have incremented continuity counters.
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

        // Inherited feedPacket() is equivalent to processPacket(const TSPacket&) without bool result.
        virtual void feedPacket(const TSPacket& pkt) override;

    protected:
        // Inherited methods.
        virtual void immediateReset() override;
        virtual void immediateResetPID(PID pid) override;

    private:
        // PID analysis state.
        class PIDState
        {
        public:
            PIDState() = default;              // Constructor
            uint8_t  first_cc = INVALID_CC;    // First CC value in a PID.
            uint8_t  last_cc_out = INVALID_CC; // Last output CC value in a PID.
            size_t   dup_count = 0;            // Consecutive duplicate count.
            TSPacket last_pkt_in {};           // Last input packet (before modification, if any).
        };

        // Private members.
        int           _severity = Severity::Info; // Severity level for error messages.
        bool          _display_errors = false;    // Display discontinuity errors.
        bool          _fix_errors = false;        // Fix discontinuity errors.
        bool          _replicate_dup = true;      // With _fix_errors, replicate duplicate packets.
        bool          _generator = false;         // Use generator mode.
        bool          _json = false;              // Log JSON messages.
        UString       _prefix {};                 // Message prefix.
        PacketCounter _fix_count = 0;             // Number of fixed (modified) packets.
        PacketCounter _error_count = 0;           // Number of discontinuity errors.
        std::map<PID,PIDState> _pid_states {};    // State of all PID's.

        // Internal version of feedPacket.
        // The packet is modified only if update is true.
        bool processPacketInternal(TSPacket* pkt, bool update);

        // Build the first part of an error message.
        UString linePrefix(PID pid) const;

        // Log a JSON message.
        void logJSON(PID pid, const UChar* type, size_t packet_count = NPOS);
    };
}
