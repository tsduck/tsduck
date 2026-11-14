//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Regulate execution speed based on PCR's in one reference PID.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractRegulator.h"

namespace ts {
    //!
    //! Regulate execution speed based on PCR's in one reference PID.
    //! @ingroup libtsduck mpeg
    //! @see BitRateRegulator
    //! @see TimeStampRegulator
    //!
    class TSDUCKDLL PCRRegulator: public AbstractRegulator
    {
        TS_NOBUILD_NOCOPY(PCRRegulator);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit PCRRegulator(Report* report, int log_level = Severity::Verbose) : AbstractRegulator(report, log_level) {}

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit PCRRegulator(ReporterBase* delegate, int log_level = Severity::Verbose) : AbstractRegulator(delegate, log_level) {}

        //!
        //! Destructor.
        //!
        virtual ~PCRRegulator() override;

        //!
        //! Set the PCR reference PID.
        //! @param [in] pid Reference PID. If PID_NULL, use the first PID containing PCR's.
        //!
        void setReferencePID(PID pid);

        //!
        //! Get the current PCR reference PID.
        //! @return Current reference PID or PID_NULL if none was set or found.
        //!
        PID getReferencePID() const { return _pid; }

        // Inherited methods.
        virtual bool reset() override;
        virtual void regulate(TSPacket& pkt, TSPacketMetadata& mdata, const BitRate& current_bitrate) override;

    private:
        PID            _user_pid = PID_NULL;      // User-specified reference PID.
        PID            _pid = PID_NULL;           // Current reference PID.
        PacketCounter  _burst_pkt_cnt = 0;        // Number of packets in current burst
        bool           _started = false;          // First PCR found, regulation started.
        uint64_t       _pcr_first = INVALID_PCR;  // First PCR value.
        uint64_t       _pcr_last = INVALID_PCR;   // Last PCR value.
        uint64_t       _pcr_offset = 0;           // Offset to add to PCR value, accumulate all PCR wrap-down sequences.
        monotonic_time _clock_first {};           // System time at first PCR.
        monotonic_time _clock_last {};            // System time at last wait
    };
}
