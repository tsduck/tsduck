//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class of all transport stream regulators.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tsTSPacket.h"
#include "tsTSPacketMetadata.h"
#include "tsSysUtils.h"

namespace ts {
    //!
    //! Abstract base class of all transport stream regulators.
    //! A regulator class processes transport stream packets and suspends the
    //! execution from time to time to force a given output speed policy.
    //! @ingroup libtsduck mpeg
    //! @see BitRateRegulator
    //! @see PCRRegulator
    //! @see TimeStampRegulator
    //!
    class TSDUCKDLL AbstractRegulator: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(AbstractRegulator);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit AbstractRegulator(Report* report, int log_level = Severity::Verbose);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] log_level Severity level for information messages.
        //!
        explicit AbstractRegulator(ReporterBase* delegate, int log_level = Severity::Verbose);

        //!
        //! Destructor.
        //!
        virtual ~AbstractRegulator() override;

        //!
        //! Set the number of packets to burst at a time.
        //! This is used as a hint for the concrete subclass. The actual burst can be higher if the system cannot wait for too small durations.
        //! @param [in] count Number of packets to burst at a time.
        //!
        void setBurstPacketCount(PacketCounter count) { _requested_burst = count; }

        //!
        //! Set a fixed bitrate, ignore @a current_bitrate parameter in regulate().
        //! The specified bitrate may be ignored by the concrete subclass.
        //! @param [in] bitrate Fixed bitrate to use. When zero, use current bitrate.
        //!
        void setFixedBitRate(const BitRate& bitrate) { _fixed_bitrate = bitrate; }

        //!
        //! Set the minimum wait interval.
        //! The specified interval may be ignored by the concrete subclass.
        //! @param [in] d Minimum duration interval.
        //!
        template <class Rep, class Period>
        void setMinimimWait(const cn::duration<Rep,Period>& d);

        //!
        //! Reset and restart regulation.
        //! @return True on success, false on error.
        //!
        virtual bool reset() = 0;

        //!
        //! Regulate the flow, to be called at each packet.
        //! Suspend the process when necessary.
        //! @param [in,out] pkt TS packet from the stream. Maybe modified if necessary by subclass.
        //! @param [in,out] mdata TS packet metadata. Maybe modified if necessary by subclass.
        //! @param [in] current_bitrate Current bitrate if available. Informational only, zero if unknown.
        //! Ignored if a fixed bitrate has been set by
        //!
        virtual void regulate(TSPacket& pkt, TSPacketMetadata& mdata, const BitRate& current_bitrate) = 0;

        //!
        //! Default minimum wait interval.
        //!
        static constexpr cn::milliseconds DEFAULT_MIN_WAIT = cn::milliseconds(2);

    protected:
        int              _log_level = Severity::Info;  //!< Severity level for information messages.
        BitRate          _fixed_bitrate = 0;           //!< Declared fixed bitrate. Zero means use @a current_bitrate parameter.
        PacketCounter    _requested_burst = 0;         //!< Requested number of packets to burst at a time.
        cn::microseconds _minimum_wait {};             //!< Minimum delay between two waits.
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template <class Rep, class Period>
void ts::AbstractRegulator::setMinimimWait(const cn::duration<Rep,Period>& d)
{
    using Duration = cn::duration<Rep,Period>;
    if (d != _minimum_wait && d > Duration::zero()) {
        // Set the timer precision to 2 seconds, or more if not possible.
        cn::microseconds precision = DEFAULT_MIN_WAIT;
        SetTimersPrecision(precision);
        // Cap the requested minimum wait with the granted timer precision.
        _minimum_wait = std::max(cn::duration_cast<cn::microseconds>(d), precision);
        report().log(_log_level, u"minimum wait: %s, using %s", precision, _minimum_wait);
    }
}
