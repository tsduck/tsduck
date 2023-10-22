//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Metadata of an MPEG-2 transport packet for tsp plugins.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsByteBlock.h"
#include "tsTimeSource.h"
#include "tsCompactBitSet.h"
#include "tsResidentBuffer.h"

namespace ts {
    //!
    //! A set of labels used as metadata for a TS packet.
    //!
    //! Each packet in a tsp processing chain has a set of "labels".
    //! A label is a integer value from 0 to 31. Various plugins may
    //! set or reset labels on packets. Any plugin can be conditioned
    //! to work on packets having specific labels only.
    //!
    //! When TS packets are passed between processes, using a pipe for
    //! instance, the labels can passed as well as other metadata using
    //! "--format duck".
    //!
    //! The type TSPacketLabelSet is crafted to use 32 bits exactly.
    //! It was previously implemented as std::bitset<32> but this
    //! type may use up to 64 bits, depending on the platform.
    //!
    //! @ingroup mpeg
    //!
    typedef CompactBitSet<32> TSPacketLabelSet;

    //!
    //! Metadata of an MPEG-2 transport packet for tsp plugins.
    //! @ingroup mpeg
    //!
    //! An instance of this class is passed with each TS packet to packet processor plugins.
    //!
    class TSDUCKDLL TSPacketMetadata final
    {
    public:
        //!
        //! Constructor.
        //!
        TSPacketMetadata();

        //!
        //! Reset the content of this instance.
        //! Return to initial empty state.
        //!
        void reset();

        //!
        //! Specify if the packet was artificially inserted as input stuffing.
        //! @param [in] on When true, the packet was artificially inserted as input stuffing.
        //!
        void setInputStuffing(bool on) { _input_stuffing = on; }

        //!
        //! Check if the packet was artificially inserted as input stuffing.
        //! @return True when the packet was artificially inserted as input stuffing.
        //!
        bool getInputStuffing() const { return _input_stuffing; }

        //!
        //! Specify if the packet was explicitly turned into a null packet by a plugin.
        //! @param [in] on When true, the packet was explicitly turned into a null packet by a plugin.
        //!
        void setNullified(bool on) { _nullified = on; }

        //!
        //! Check if the packet was explicitly turned into a null packet by a plugin.
        //! @return True when the packet was explicitly turned into a null packet by a plugin.
        //!
        bool getNullified() const { return _nullified; }

        //!
        //! Specify if the packet chain shall be flushed by tsp as soon as possible.
        //! This is typically called by a packet processing plugin.
        //! @param [in] on When set to true by a packet processing plugin, the packet and all previously
        //! processed and buffered packets should be passed to the next processor as soon as possible.
        //!
        void setFlush(bool on) { _flush = on; }

        //!
        //! Check if the packet chain shall be flushed by tsp as soon as possible.
        //! @return True when the packet and all previously processed and buffered packets should be
        //! passed to the next processor as soon as possible
        //!
        bool getFlush() const { return _flush; }

        //!
        //! Specify if the plugin has changed the transport stream bitrate.
        //! This is typically called by a packet processing plugin.
        //! @param [in] on When set to true by a packet processing plugin, tsp should call its
        //! getBitrate() callback as soon as possible.
        //!
        void setBitrateChanged(bool on) { _bitrate_changed = on; }

        //!
        //! Check if the plugin has changed the transport stream bitrate.
        //! @return True when tsp should call the getBitrate() callback of the plugin as soon as possible.
        //!
        bool getBitrateChanged() const { return _bitrate_changed; }

        //!
        //! Check if the TS packet has a specific label set.
        //! @param [in] label The label to check.
        //! @return True if the TS packet has @a label set.
        //!
        bool hasLabel(size_t label) const { return _labels.test(label); }

        //!
        //! Check if the TS packet has any label set.
        //! @return True if the TS packet has any label.
        //!
        bool hasAnyLabel() const { return _labels.any(); }

        //!
        //! Check if the TS packet has any label set from a set of labels.
        //! @param [in] mask The mask of labels to check.
        //! @return True if the TS packet has any label from @a mask.
        //!
        bool hasAnyLabel(const TSPacketLabelSet& mask) const { return (_labels & mask).any(); }

        //!
        //! Check if the TS packet has all labels set from a set of labels.
        //! @param [in] mask The mask of labels to check.
        //! @return True if the TS packet has all labels from @a mask.
        //!
        bool hasAllLabels(const TSPacketLabelSet& mask) const { return (_labels & mask) == mask; }

        //!
        //! Set a specific label for the TS packet.
        //! @param [in] label The label to set.
        //!
        void setLabel(size_t label) { _labels.set(label); }

        //!
        //! Set a specific set of labels for the TS packet.
        //! @param [in] mask The mask of labels to set.
        //!
        void setLabels(const TSPacketLabelSet& mask) { _labels |= mask; }

        //!
        //! Clear a specific label for the TS packet.
        //! @param [in] label The label to clear.
        //!
        void clearLabel(size_t label) { _labels.reset(label); }

        //!
        //! Clear a specific set of labels for the TS packet.
        //! @param [in] mask The mask of labels to clear.
        //!
        void clearLabels(const TSPacketLabelSet& mask) { _labels &= ~mask; }

        //!
        //! Clear all labels for the TS packet.
        //!
        void clearAllLabels() { _labels.reset(); }

        //!
        //! Get the list of labels as a string, typically for debug messages.
        //! @param [in] separator Separator between labale values.
        //! @param [in] none String to display when there is no label set.
        //! @return List of labels as a string.
        //!
        UString labelsString(const UString& separator = u" ", const UString& none = u"none") const;

        //!
        //! Get the optional input time stamp of the packet.
        //! @return The input time stamp in PCR units (27 MHz) or INVALID_PCR if there is none.
        //! - The input time stamp is optional. It may be set by the input plugin or by @c tsp
        //!   or not set at all.
        //! - Its precision, accuracy and reliability are unspecified. It may be set by @c tsp
        //!   software (based on internal clock), by the receiving hardware (the NIC for instance)
        //!   or by some external source (RTP or M2TS time stamp).
        //! - It is a monotonic clock which wraps up after MAX_PCR (at least).
        //! - It can also wrap up at any other input-specific value. For instance, M2TS files use 30-bit
        //!   timestamps in PCR units. So, for M2TS the input time stamps wrap up every 39 seconds.
        //! - Although expressed in PCR units, it does not share the same reference clock with the
        //!   various PCR in the transport stream. You can compare time stamp differences, not
        //!   absolute values.
        //!
        uint64_t getInputTimeStamp() const { return _input_time; }

        //!
        //! Get the identification of the source of the input time stamp.
        //! @return An identifier for the source of the input time stamp.
        //!
        TimeSource getInputTimeSource() const { return _time_source; }

        //!
        //! Check if the packet has an input time stamp.
        //! @return True if the packet has an input time stamp.
        //!
        bool hasInputTimeStamp() const { return _input_time != INVALID_PCR; }

        //!
        //! Clear the input time stamp.
        //!
        void clearInputTimeStamp();

        //!
        //! Set the optional input time stamp of the packet.
        //! @param [in] time_stamp Input time stamp value. This value should be taken from a
        //! monotonic clock. The time unit is specified in @a ticks_per_second.
        //! @param [in] ticks_per_second Base unit of the @a time_stamp value.
        //! This is the number of units per second. For instance, @a ticks_per_second
        //! should be 1000 when @a time_stamp is in milliseconds and it should be
        //! @link SYSTEM_CLOCK_FREQ @endlink when @a time_stamp is in PCR units.
        //! If @a ticks_per_second is zero, then the input time stamp is cleared.
        //! @param [in] source Identification of time stamp source.
        //! @see getInputTimeStamp()
        //!
        void setInputTimeStamp(uint64_t time_stamp, uint64_t ticks_per_second, TimeSource source);

        //!
        //! Get the input time stamp as a string, typically for debug messages.
        //! @param [in] none String to display when there is no label set.
        //! @return Input time stamp as a string.
        //!
        UString inputTimeStampString(const UString& none = u"none") const;

        //!
        //! Copy contiguous TS packet metadata.
        //! @param [out] dest Address of the first contiguous TS packet metadata to write.
        //! @param [in] source Address of the first contiguous TS packet metadata to read.
        //! @param [in] count Number of TS packet metadata to copy.
        //!
        static void Copy(TSPacketMetadata* dest, const TSPacketMetadata* source, size_t count);

        //!
        //! Reset contiguous TS packet metadata.
        //! @param [out] dest Address of the first contiguous TS packet metadata to reset.
        //! @param [in] count Number of TS packet metadata to copy.
        //!
        static void Reset(TSPacketMetadata* dest, size_t count);

        //!
        //! Size in bytes of the structure into which a TSPacketMetadata can be serialized.
        //!
        static constexpr size_t SERIALIZATION_SIZE = 14;

        //!
        //! First "magic" byte of the structure into which a TSPacketMetadata was serialized.
        //! Intentionally the opposite of the TS packet synchro byte.
        //!
        static constexpr uint8_t SERIALIZATION_MAGIC = SYNC_BYTE ^ 0xFF;

        //!
        //! Serialize the content of this instance into a byteblock.
        //! The serialized data is a fixed size block of @link SERIALIZATION_SIZE @endlink bytes.
        //! @param [out] bin Returned binary data.
        //!
        void serialize(ByteBlock& bin) const;

        //!
        //! Serialize the content of this instance into a memory area.
        //! The serialized data is a fixed size block of @link SERIALIZATION_SIZE @endlink bytes.
        //! @param [out] data Address of the memory area.
        //! @param [in] size Size in bytes of the memory area.
        //! @return Size in bytes of the data, zero on error (buffer too short).
        //!
        size_t serialize(void* data, size_t size) const;

        //!
        //! Deserialize the content of this instance from a byteblock.
        //! @param [in] bin Binary data containing a serialized instance of TSPacketMetadata.
        //! @return True if everything has been deserialized. False if the input data block is
        //! too short. In the latter case, the missing field get their default value.
        //!
        bool deserialize(const ByteBlock& bin) { return deserialize(bin.data(), bin.size()); }

        //!
        //! Deserialize the content of this instance from a byteblock.
        //! @param [in] data Address of binary data containing a serialized instance of TSPacketMetadata.
        //! @param [in] size Size in bytes of these data.
        //! @return True if everything has been deserialized. False if the input data block is
        //! too short. In the latter case, the missing field get their default value.
        //!
        bool deserialize(const void* data, size_t size);

        //!
        //! Display the structure layout of the data structure (for debug only).
        //! @param [in,out] out Output stream, where to display.
        //! @param [in] prefix Optional prefix on each line.
        //!
        static void DisplayLayout(std::ostream& out, const char* prefix = "");

    private:
        uint64_t         _input_time;           // 64 bits: Input timestamp in PCR units, INVALID_PCR if unknown.
        TSPacketLabelSet _labels;               // 32 bits: Bit mask of labels.
        TimeSource       _time_source;          // 8 bits: Source for time stamps.
        bool             _flush : 1;            // Flush the packet buffer asap.
        bool             _bitrate_changed : 1;  // Call getBitrate() callback as soon as possible.
        bool             _input_stuffing : 1;   // Packet was artificially inserted as input stuffing.
        bool             _nullified : 1;        // Packet was explicitly turned into a null packet by a plugin.
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(unused-private-field)
        unsigned int     _pad1 : 4;             // Padding to next byte.
        unsigned int     _pad2 : 16;            // Padding to next multiple of 4 bytes.
        TS_POP_WARNING()
    };

    //!
    //! Vector of packet metadata.
    //!
    typedef std::vector<TSPacketMetadata> TSPacketMetadataVector;

    //!
    //! Metadata for TS packet are accessed in a memory-resident buffer.
    //! A packet and its metadata have the same index in their respective buffer.
    //!
    typedef ResidentBuffer<TSPacketMetadata> PacketMetadataBuffer;
}
