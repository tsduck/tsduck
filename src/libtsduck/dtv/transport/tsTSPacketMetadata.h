//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
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

    class DuckContext;

    //!
    //! A set of labels used as metadata for a TS packet.
    //! @ingroup mpeg
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
    using TSPacketLabelSet = CompactBitSet<32>;

    //!
    //! Metadata of an MPEG-2 transport packet for tsp plugins.
    //! @ingroup libtsduck mpeg
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
        //! Specify if the packet was originally extracted from a datagram of several TS packets.
        //! @param [in] on When true, the packet was originally extracted from a datagram of several TS packets.
        //!
        void setDatagram(bool on) { _datagram = on; }

        //!
        //! Check if the packet was originally extracted from a datagram of several TS packets.
        //! @return True when the packet was originally extracted from a datagram of several TS packets.
        //!
        bool getDatagram() const { return _datagram; }

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
        //! Get all labels from the TS packet.
        //! @return The set of all labels from the TS packet.
        //!
        TSPacketLabelSet labels() const { return _labels; }

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
        PCR getInputTimeStamp() const { return PCR(_input_time); }

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
        //! @param [in] time_stamp Input time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] source Identification of time stamp source.
        //! @see getInputTimeStamp()
        //!
        template <class Rep, class Period>
        void setInputTimeStamp(const cn::duration<Rep,Period>& time_stamp, TimeSource source);

        //!
        //! Get the input time stamp as a string, typically for debug messages.
        //! @param [in] none String to display when there is no label set.
        //! @return Input time stamp as a string.
        //!
        UString inputTimeStampString(const UString& none = u"none") const;

        //!
        //! Maximum size in bytes of auxiliary data.
        //! @see setAuxData()
        //!
        static constexpr size_t AUX_DATA_MAX_SIZE = 16;

        //!
        //! Copy bytes into the auxiliary data.
        //! The auxiliary data are made of up to 16 bytes and are attached to the packet metadata.
        //! There is no predefined usage for auxiliary data. In practice, they are read from the
        //! 16-byte trailer which can be found in some modulation systems, after the TS packet.
        //! @param data [in] Address of data to copy into the auxiliary data.
        //! @param size [in] Size in bytes of the data to copy. Up to 16 bytes are used, the rest is ignored.
        //!
        void setAuxData(const void* data, size_t size);

        //!
        //! Copy bytes from the auxiliary data.
        //! @param data [out] Address of user buffer to receive the auxiliary data.
        //! @param max_size [in] Maximum size in bytes of the user buffer.
        //! @return Number of copied bytes. Usually 0 or 16. Never more than 16.
        //! @see setAuxData()
        //!
        size_t getAuxData(void* data, size_t max_size) const;

        //!
        //! Copy bytes from the auxiliary data and pad user buffer if necessary.
        //! @param data [out] Address of user buffer to receive the auxiliary data.
        //! @param max_size [in] Maximum size in bytes of the user buffer.
        //! @param pad [in] Pad value to copy in all additional bytes in the user's buffer
        //! if the auxiliary data are shorter than @a max_size.
        //! @see setAuxData()
        //!
        void getAuxData(void* data, size_t max_size, uint8_t pad) const;

        //!
        //! Size in bytes of the auxiliary data.
        //! @return The size in bytes of the auxiliary data.
        //! @see setAuxData()
        //!
        size_t auxDataSize() const { return _aux_data_size; }

        //!
        //! Direct access to the auxiliary data.
        //! @return Address of the auxiliary data inside this object.
        //! @see setAuxData()
        //!
        const uint8_t* auxData() const { return _aux_data; }

        //!
        //! Direct access to the auxiliary data.
        //! @return Address of the auxiliary data inside this object.
        //! @see setAuxData()
        //!
        uint8_t* auxData() { return _aux_data; }

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
        //! The auxiliary data are not included in the serialized buffer.
        //! @param [out] bin Returned binary data.
        //!
        void serialize(ByteBlock& bin) const;

        //!
        //! Serialize the content of this instance into a memory area.
        //! The serialized data is a fixed size block of @link SERIALIZATION_SIZE @endlink bytes.
        //! The auxiliary data are not included in the serialized buffer.
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
        bool             _datagram : 1;         // Packet was originally extracted from a datagram of several TS packets.
        TS_PUSH_WARNING()
        TS_LLVM_NOWARNING(unused-private-field)
        unsigned int     _pad1 : 3;             // Padding to next byte.
        unsigned int     _pad2 : 8;             // Padding to next multiple of 4 bytes -1.
        TS_POP_WARNING()
        uint8_t          _aux_data_size;        // Number of used bytes in _aux_data.
        uint8_t          _aux_data[AUX_DATA_MAX_SIZE]; // Auxiliary data (16 bytes).
    };

    //!
    //! Vector of packet metadata.
    //!
    using TSPacketMetadataVector = std::vector<TSPacketMetadata>;

    //!
    //! Metadata for TS packet are accessed in a memory-resident buffer.
    //! A packet and its metadata have the same index in their respective buffer.
    //!
    using PacketMetadataBuffer = ResidentBuffer<TSPacketMetadata>;
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Set the optional input time stamp of the packet.
template <class Rep, class Period>
void ts::TSPacketMetadata::setInputTimeStamp(const cn::duration<Rep,Period>& time_stamp, TimeSource source)
{
    _time_source = source;
    const PCR pcr = cn::duration_cast<PCR>(time_stamp);
    // Make sure we remain in the usual PCR range.
    // This can create an issue if the input value wraps up at 2^64.
    // In which case, the PCR value will warp at another value than PCR_SCALE.
    _input_time = uint64_t(pcr.count() % PCR_SCALE);
}

#endif // DOXYGEN
