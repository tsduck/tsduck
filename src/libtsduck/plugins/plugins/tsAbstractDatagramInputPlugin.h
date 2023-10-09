//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Abstract base class for input plugins receiving real-time datagrams.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsInputPlugin.h"
#include "tsTSPacketMetadata.h"
#include "tsByteBlock.h"
#include "tsEnumeration.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Abstract base class for input plugins receiving real-time datagrams.
    //! The input bitrate is computed from the received bytes and wall-clock time.
    //! TS packets are located in each received datagram, skipping potential headers.
    //! @ingroup plugin
    //!
    class TSDUCKDLL AbstractDatagramInputPlugin: public InputPlugin
    {
        TS_NOBUILD_NOCOPY(AbstractDatagramInputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool isRealTime() override;
        virtual BitRate getBitrate() override;
        virtual BitRateConfidence getBitrateConfidence() override;
        virtual size_t receive(TSPacket*, TSPacketMetadata*, size_t) override;

    protected:
        //!
        //! Constructor for subclasses.
        //! @param [in] tsp Associated callback to @c tsp executable.
        //! @param [in] buffer_size Size in bytes of input buffer.
        //! Must be large enough to contain the largest datagram.
        //! @param [in] description A short one-line description, eg. "Wonderful File Copier".
        //! @param [in] syntax A short one-line syntax summary, eg. "[options] filename ...".
        //! @param [in] system_time_name When the subclass provides timestamps, this is a lowercase name
        //! which is used in option -\-timestamp-priority. When empty, there is no timestamps from the subclass.
        //! @param [in] system_time_description Description of @a system_time_name for help text.
        //! @param [in] real_time If true, the reception occurs in real-time, typically from
        //! the network. When false, the "reception" can be reading a capture file.
        //!
        AbstractDatagramInputPlugin(TSP* tsp,
                                    size_t buffer_size,
                                    const UString& description,
                                    const UString& syntax,
                                    const UString& system_time_name,
                                    const UString& system_time_description,
                                    bool real_time);

        //!
        //! Receive a datagram message.
        //! Must be implemented by subclasses.
        //! @param [out] buffer Address of the buffer for the received message.
        //! @param [in] buffer_size Size in bytes of the reception buffer.
        //! @param [out] ret_size Size in bytes of the received message. Will never be larger than @a buffer_size.
        //! @param [out] timestamp Receive timestamp in micro-seconds or -1 if not available.
        //! @return True on success, false on error.
        //!
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp) = 0;

    private:
        // Order of priority for input timestamps. SYSTEM means lower layer from subclass (UDP, SRT, etc).
        enum TimePriority {RTP_SYSTEM_TSP, SYSTEM_RTP_TSP, RTP_TSP, SYSTEM_TSP, TSP_ONLY};

        // Configuration and command line options.
        bool          _real_time;             // Real-time reception.
        MilliSecond   _eval_time;             // Bitrate evaluation interval in milli-seconds
        MilliSecond   _display_time;          // Bitrate display interval in milli-seconds
        Enumeration   _time_priority_enum;    // Enumeration values for _time_priority
        TimePriority  _time_priority;         // Priority of time stamps sources.
        TimePriority  _default_time_priority; // Priority of time stamps sources.

        // Working data.
        Time          _next_display;          // Next bitrate display time
        Time          _start;                 // UTC date of first received packet
        PacketCounter _packets;               // Number of received packets since _start
        Time          _start_0;               // Start of previous bitrate evaluation period
        PacketCounter _packets_0;             // Number of received packets since _start_0
        Time          _start_1;               // Start of previous bitrate evaluation period
        PacketCounter _packets_1;             // Number of received packets since _start_1
        size_t        _inbuf_count;           // Number of remaining TS packets in inbuf
        size_t        _inbuf_next;            // Byte index in _inbuf of next TS packet to return
        size_t        _mdata_next;            // Index in _mdata of next TS packet metadata to return
        ByteBlock     _inbuf;                 // Input buffer
        TSPacketMetadataVector _mdata;        // Metadata for packets in _inbuf
    };
}
