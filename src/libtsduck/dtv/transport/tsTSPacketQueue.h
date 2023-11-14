//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Transport stream packet queue for inter-thread communication.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTSPacket.h"
#include "tsPCRAnalyzer.h"

namespace ts {
    //!
    //! Transport stream packet queue for inter-thread communication.
    //! @ingroup mpeg
    //!
    //! A writer thread produces packets. The input packets are directly written
    //! into the buffer. The writer thread invokes lockWriteBuffer() to get
    //! a write window inside the buffer. When packets have been written into
    //! this buffer, the writer thread calls releaseWriteBuffer().
    //!
    //! A reader thread consumes packets. The packets are consumed one by one
    //! and copied out of the buffer using getPacket().
    //!
    //! The input bitrate, if known, is transmitted to the reader thread. If the
    //! writer thread is aware of the exact bitrate, it calls setBitrate() and
    //! the specified value is returned to the reader thread. If the input bitrate
    //! is unknown, the buffer automatically computes it based on PCR's.
    //!
    //! Termination conditions can be triggered on both sides.
    //!
    class TSDUCKDLL TSPacketQueue
    {
        TS_NOCOPY(TSPacketQueue);
    public:
        //!
        //! Default size in packets of the buffer.
        //!
        static constexpr size_t DEFAULT_SIZE = 1000;

        //!
        //! Default constructor.
        //! @param [in] size Size of the buffer in packets.
        //!
        TSPacketQueue(size_t size = DEFAULT_SIZE);

        //!
        //! Reset and resize the buffer.
        //! It is illegal to reset the buffer while the writer thread has locked the buffer.
        //! This is not enforced by this class. It is the responsibility of the application to check this.
        //! @param [in] size New size of the buffer in packets. By default, when set to NPOS,
        //! reset the queue without resizing the buffer.
        //!
        void reset(size_t size = NPOS);

        //!
        //! Get the size of the buffer in packets.
        //! @return The size of the buffer in packets.
        //!
        size_t bufferSize() const;

        //!
        //! Get the current number of packets in the buffer.
        //! @return The current number of packets in the buffer.
        //!
        size_t currentSize() const;

        //!
        //! Called by the writer thread to get a write buffer.
        //! The writer thread is suspended until enough free space is made in the buffer
        //! or the reader thread triggers a stop condition.
        //! @param [out] buffer Address of the write buffer.
        //! @param [out] buffer_size Size in packets of the write buffer.
        //! @param [in] min_size Minimum number of free packets to get. This is just a
        //! hint. The returned size can be smaller, for instance when the write window
        //! of the circular buffer is close to the end of the buffer.
        //! @return True when the write buffer is correctly available.
        //! False when the reader thread has signalled a stop condition.
        //!
        bool lockWriteBuffer(TSPacket*& buffer, size_t& buffer_size, size_t min_size = 1);

        //!
        //! Called by the writer thread to release the write buffer.
        //! The packets were written by the writer thread at the address which
        //! was returned by lockWriteBuffer().
        //! @param [in] count Number of packets which were written in the buffer.
        //! Must be no greater than the size which was returned by lockWriteBuffer().
        //!
        void releaseWriteBuffer(size_t count);

        //!
        //! Called by the writer thread to report the input bitrate.
        //! @param [in] bitrate Input bitrate. If zero, the input bitrate is unknown
        //! and will be computed from PCR's.
        //!
        void setBitrate(const BitRate& bitrate);

        //!
        //! Called by the writer thread to report the end of input thread.
        //!
        void setEOF();

        //!
        //! Check if the reader thread has reported a stop condition.
        //! @return True if the reader thread has reported a stop condition.
        //!
        bool stopped() const { return _stopped; }

        //!
        //! Called by the reader thread to get the next packet without waiting.
        //! The reader thread is never suspended. If no packet is available, return false.
        //! @param [out] packet The returned packet. Unmodified when no packet is available.
        //! @param [out] bitrate Input bitrate or zero if unknown.
        //! @return True if a packet was returned in @a packet. False if none was available
        //! or an end of file occured.
        //!
        bool getPacket(TSPacket& packet, BitRate& bitrate);

        //!
        //! Called by the reader thread to wait for packets.
        //! The reader thread is suspended until at least one packet is available.
        //! @param [out] buffer Address of packet buffer.
        //! @param [in] buffer_count Size of @a buffer in number of packets.
        //! @param [out] actual_count Number of returned packets in @a buffer.
        //! @param [out] bitrate Input bitrate or zero if unknown.
        //! @return True if a packets were returned in @a buffer. False on error or end of file.
        //!
        bool waitPackets(TSPacket* buffer, size_t buffer_count, size_t& actual_count, BitRate& bitrate);

        //!
        //! Check if the writer thread has reported an end of file condition.
        //! @return True if the writer thread has reported an end of file condition.
        //!
        bool eof() const;

        //!
        //! Called by the reader thread to tell the writer thread to stop immediately.
        //!
        void stop();

    private:
        volatile bool     _eof = false;      // The writer thread has reported an end of file.
        volatile bool     _stopped = false;  // The read thread has reported a stop condition.
        mutable std::mutex              _mutex {};     // Protect access to shared data.
        mutable std::condition_variable _enqueued {};  // Signaled when packets are inserted.
        mutable std::condition_variable _dequeued {};  // Signaled when packets were freed.
        TSPacketVector    _buffer {};        // The packet buffer.
        PCRAnalyzer       _pcr {1, 12};      // PCR analyzer to get the bitrate.
        size_t            _inCount = 0;      // Number of packets currently inside the buffer.
        size_t            _readIndex = 0;    // Index of next packet to read.
        size_t            _writeIndex = 0;   // Index of next packet to write.
        BitRate           _bitrate = 0;      // Bitrate as set by the writer thread.

        // Get bitrate, must be called with mutex held.
        BitRate getBitrate() const;
    };
}
