//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Control the insertion points of TS packets in a stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNullReport.h"
#include "tsTSPacket.h"

namespace ts {
    //!
    //! Control the insertion points of TS packets in a stream based on various criteria.
    //! @ingroup mpeg
    //!
    //! The scenarion is the following:
    //! - The main transport stream has some known bitrate.
    //! - A sub-stream shall be inserted in the main stream (one PID, a merged TS, whatever).
    //! - The sub-stream has a known target bitrate inside the main stream.
    //! - We count packets in the main TS.
    //! - We want to know when we should insert packets from the sub-stream inside the main stream.
    //!
    class TSDUCKDLL PacketInsertionController
    {
        TS_NOCOPY(PacketInsertionController);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report verbose and debug messages.
        //!
        PacketInsertionController(Report& report = NULLREP);

        //!
        //! Reset the state of the controller.
        //! The packet counters are reset.
        //! The last bitrates and stream names are retained.
        //!
        void reset();

        //!
        //! Declare a new value for the bitrate of the main transport stream.
        //! @param [in] rate New bitrate value. Zero means unknown bitrate.
        //!
        void setMainBitRate(const BitRate& rate);

        //!
        //! Declare a new value for the bitrate of the sub-stream.
        //! @param [in] rate New bitrate value. Zero means unknown bitrate.
        //!
        void setSubBitRate(const BitRate& rate);

        //!
        //! Get current bitrate of the main transport stream.
        //! @return Current bitrate of the main transport stream.
        //!
        BitRate currentMainBitRate() const { return _main_bitrate.getBitRate(); }

        //!
        //! Get current bitrate of the sub-stream.
        //! @return Current bitrate of the sub-stream.
        //!
        BitRate currentSubBitRate() const { return _sub_bitrate.getBitRate(); }

        //!
        //! Count packets in the main transport stream.
        //! This method must be called each time packets pass through the main transport stream.
        //! This includes packets which are replaced with content from the sub-stream.
        //! @param [in] packets Number of passed packets in the main transport stream.
        //!
        void declareMainPackets(size_t packets) { _main_packets += packets; }

        //!
        //! Count packets in the sub-stream.
        //! This method must be called each time packets from the main transport stream
        //! are replaced with content from the sub-stream.
        //! @param [in] packets Number of packets which are replaced with content from the sub-stream.
        //!
        void declareSubPackets(size_t packets) { _sub_packets += packets; }

        //!
        //! Check if a packet from the sub-stream shall be inserted at the current position in the main transport stream.
        //! @param [in] waiting_packets Number of packets waiting to be inserted. This value is used only when greater
        //! than 1. In that case, it indicates that a real-time source is providing packets in bursts. If the bursts
        //! are too important, then packet insertion is temporarily accelerated to avoid overflow.
        //! @return True if it is appropriate to insert a packet from the sub-stream here, false otherwise.
        //! Always return true if any bitrate, main stream or sub-stream, is ignored (zero).
        //! @see setWaitPacketsAlertThreshold()
        //!
        bool mustInsert(size_t waiting_packets = 1);

        //!
        //! Set an alert threshold to waiting packets.
        //!
        //! When the number of current packets waiting to be inserted from the sub-stream becomes higher
        //! than the specified alert threshold, the insertion rate is temporarily accelerated to avoid overflow.
        //! When the sub-stream is correctly regulated on average but regularly bursts, the waiting packets
        //! threshold should be at least the burst size to keep a smooth insertion.
        //!
        //! This parameter is used to make sure that 1) bursts do not affect the insertion rythm and
        //! 2) overflows are smoothly absorbed.
        //!
        //! The initial default threshold is DEFAULT_WAIT_ALERT packets.
        //! @param [in] packets New alert threshold, in number of packets. When set to zero,
        //! no acceleration is performed.
        //! @see mustInsert()
        //!
        void setWaitPacketsAlertThreshold(size_t packets) { _wait_alert = packets; }

        //!
        //! Set a reset threshold for bitrate variation.
        //!
        //! The bitrate, main stream or sub-stream, may vary. The actual bitrate which is used in computations
        //! is the average of all previously specified bitrate values. But if a bitrate suddenly varies from
        //! this average value by more than a given percentage, the average is reset to the new value.
        //! When the bitrate is reset that way, the insertion computation restarts.
        //!
        //! The initial default threshold is DEFAULT_BITRATE_RESET_PERCENT.
        //! @param [in] percent New percentage threshold (0 to 100).
        //!
        void setBitRateVariationResetThreshold(size_t percent);

        //!
        //! Set a name for the main stream (only for debug messages).
        //! @param [in] name The new name to use.
        //!
        void setMainStreamName(const UString& name) { _main_name = name; }

        //!
        //! Set a name for the sub-stream (only for debug messages).
        //! @param [in] name The new name to use.
        //!
        void setSubStreamName(const UString& name) { _sub_name = name; }

        //!
        //! Default alert threshold for packets waiting from the sub-stream.
        //! @see setWaitPacketsAlertThreshold()
        //!
        static constexpr size_t DEFAULT_WAIT_ALERT = 16;

        //!
        //! Default reset threshold for bitrate variation (percentage).
        //! @see setBitRateVariationResetThreshold()
        //!
        static constexpr size_t DEFAULT_BITRATE_RESET_PERCENT = 10;

    private:
        // This class computes a bitrate based on all its successive values.
        // The semantics of each call is the same as in the public interface.
        class BitRateControl
        {
            TS_NOBUILD_NOCOPY(BitRateControl);
        public:
            BitRateControl(Report& report, const UString& name);
            bool setBitRate(const BitRate& rate); // return false on reset
            BitRate getBitRate() const { return _average; }
            void setResetThreshold(size_t percent) { _reset_percent = percent; }
        private:
            size_t diffPercent(const BitRate& rate) const;
        private:
            Report&        _report;
            const UString& _name;
            int64_t        _count = 0;
            BitRate        _value_0 = 0;
            BitRate        _diffs = 0;
            BitRate        _average = 0;
            size_t         _reset_percent {DEFAULT_BITRATE_RESET_PERCENT};
        };

        // PacketInsertionController private members.
        Report&        _report;                      // Where to report messages.
        UString        _main_name {u"main stream"};  // Name of main stream.
        UString        _sub_name {u"sub-stream"};    // Name of sub-stream.
        PacketCounter  _main_packets = 0;            // Total number of packets in main stream so far.
        PacketCounter  _sub_packets = 0;             // Total number of packets in sub-stream so far.
        size_t         _wait_alert = DEFAULT_WAIT_ALERT;  // Accelerate insertion above that number of waiting packets.
        size_t         _accel_factor = 1;            // Acceleration factor, greater than 1 when too many packets are waiting.
        PacketCounter  _accel_main_packets = 0;      // Number of packets in main stream when current acceleration started.
        PacketCounter  _accel_sub_packets = 0;       // Number of packets in sub-stream wehn current acceleration started.
        size_t         _accel_max_wait = 0;          // Maximum number of waiting packet in current acceleration phase.
        BitRateControl _main_bitrate {_report, _main_name};  // Current bitrate in main stream.
        BitRateControl _sub_bitrate {_report, _sub_name};    // Current bitrate in sub-stream.
    };
}
