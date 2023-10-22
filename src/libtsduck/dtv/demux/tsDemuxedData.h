//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Base class for all kinds of demuxed data.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsByteBlock.h"

namespace ts {
    //!
    //! Base class for all kinds of demuxed data.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL DemuxedData
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        DemuxedData(PID source_pid = PID_NULL);

        //!
        //! Copy constructor.
        //! @param [in] other Another instance to copy.
        //! @param [in] mode The data are either shared (ShareMode::SHARE) between the
        //! two instances or duplicated (ShareMode::COPY).
        //!
        DemuxedData(const DemuxedData& other, ShareMode mode);

        //!
        //! Move constructor.
        //! @param [in,out] other Another instance to move.
        //!
        DemuxedData(DemuxedData&& other) noexcept;

        //!
        //! Constructor from full binary content.
        //! @param [in] content Address of the binary data.
        //! @param [in] content_size Size in bytes of the data.
        //! @param [in] source_pid PID from which the data was read.
        //!
        DemuxedData(const void* content, size_t content_size, PID source_pid = PID_NULL);

        //!
        //! Constructor from full binary content.
        //! @param [in] content Binary packet data.
        //! @param [in] source_pid PID from which the packet was read.
        //!
        DemuxedData(const ByteBlock& content, PID source_pid = PID_NULL);

        //!
        //! Constructor from full binary content.
        //! @param [in] content_ptr Safe pointer to the binary data.
        //! The content is referenced, and thus shared.
        //! Do not modify the referenced ByteBlock from outside the DemuxedData.
        //! @param [in] source_pid PID from which the data were read.
        //!
        DemuxedData(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL);

        //!
        //! Virtual destructor.
        //!
        virtual ~DemuxedData();

        //!
        //! Reload from full binary content.
        //! @param [in] content Address of the binary packet data.
        //! @param [in] content_size Size in bytes of the packet.
        //! @param [in] source_pid PID from which the data were read.
        //!
        virtual void reload(const void* content, size_t content_size, PID source_pid = PID_NULL);

        //!
        //! Reload from full binary content.
        //! @param [in] content Binary packet data.
        //! @param [in] source_pid PID from which the data were read.
        //!
        virtual void reload(const ByteBlock& content, PID source_pid = PID_NULL);

        //!
        //! Reload from full binary content.
        //! @param [in] content_ptr Safe pointer to the binary packet data.
        //! The content is referenced, and thus shared.
        //! Do not modify the referenced ByteBlock from outside the DemuxedData.
        //! @param [in] source_pid PID from which the data were read.
        //!
        virtual void reload(const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL);

        //!
        //! Clear data content.
        //!
        virtual void clear();

        //!
        //! Assignment operator.
        //! The packets are referenced, and thus shared between the two packet objects.
        //! @param [in] other Other packet to assign to this object.
        //! @return A reference to this object.
        //!
        DemuxedData& operator=(const DemuxedData& other);

        //!
        //! Move assignment operator.
        //! @param [in,out] other Other packet to assign to this object.
        //! @return A reference to this object.
        //!
        DemuxedData& operator=(DemuxedData&& other) noexcept;

        //!
        //! Duplication.
        //! Similar to assignment but the data are duplicated.
        //! @param [in] other Other data to duplicate into this object.
        //! @return A reference to this object.
        //!
        DemuxedData& copy(const DemuxedData& other);

        //!
        //! Equality operator.
        //! The source PID's are ignored, only the data contents are compared.
        //! @param [in] other Other packet to compare.
        //! @return True if the two packets are identical. False otherwise.
        //!
        bool operator==(const DemuxedData& other) const;
        TS_UNEQUAL_OPERATOR(DemuxedData)

        //!
        //! Get the source PID.
        //! @return The source PID.
        //!
        PID sourcePID() const { return _source_pid; }

        //!
        //! Set the source PID.
        //! @param [in] pid The source PID.
        //!
        void setSourcePID(PID pid) { _source_pid = pid; }

        //!
        //! Index of first TS packet of the data in the demultiplexed stream.
        //! Usually valid only if the data were extracted by a demux.
        //! @return The first TS packet of the data in the demultiplexed stream.
        //!
        PacketCounter firstTSPacketIndex() const { return _first_pkt; }

        //!
        //! Index of last TS packet of the data in the demultiplexed stream.
        //! Usually valid only if the data were extracted by a demux.
        //! @return The last TS packet of the data in the demultiplexed stream.
        //!
        PacketCounter lastTSPacketIndex() const { return _last_pkt; }

        //!
        //! Set the first TS packet of the data in the demultiplexed stream.
        //! @param [in] i The first TS packet of the data in the demultiplexed stream.
        //!
        void setFirstTSPacketIndex(PacketCounter i) { _first_pkt = i; }

        //!
        //! Set the last TS packet of the data in the demultiplexed stream.
        //! @param [in] i The last TS packet of the data in the demultiplexed stream.
        //!
        void setLastTSPacketIndex(PacketCounter i) { _last_pkt = i; }

        //!
        //! Access to the full binary content of the data.
        //! Do not modify content.
        //! @return Address of the full binary content of the data.
        //! May be invalidated after modification.
        //!
        virtual const uint8_t* content() const;

        //!
        //! Size of the logical binary content of the data.
        //! For subclasses of DemuxedData, this is the logical size of the data structure inside the DemuxedData blob.
        //! @return Size of the logical binary content of the data.
        //!
        virtual size_t size() const;

        //!
        //! Size of the complete binary raw data containing the logical structure.
        //! @return Size of the complete binary raw data.
        //!
        size_t rawDataSize() const;

        //!
        //! Check if the start of the data matches a given pattern.
        //! @param [in] pattern A byte block to compare with the start of the data.
        //! @param [in] mask Optional mask to select meaningful bits in @a pattern.
        //! @return Size of the binary content of the data.
        //!
        bool matchContent(const ByteBlock& pattern, const ByteBlock& mask = ByteBlock()) const;

    protected:
        //!
        //! Read/write access to the full binary content of the data for subclasses.
        //! @return Address of the full binary content of the data.
        //!
        uint8_t* rwContent() { return _data.isNull() ? nullptr : _data->data(); }

        //!
        //! Resize the full binary content of the data for subclasses.
        //! @param [in] s New size in bytes of the full binary content of the data.
        //!
        void rwResize(size_t s);

        //!
        //! Append raw data to the full binary content of the data for subclasses.
        //! @param [in] data Address of the new area to append.
        //! @param [in] dsize Size of the area to append.
        //!
        void rwAppend(const void* data, size_t dsize);

    private:
        // Private fields
        PID           _source_pid = PID_NULL;  // Source PID (informational)
        PacketCounter _first_pkt = 0;          // Index of first packet in stream
        PacketCounter _last_pkt = 0;           // Index of last packet in stream
        ByteBlockPtr  _data {};                // Full binary content of the packet

        // Inaccessible operations
        DemuxedData(const DemuxedData&) = delete;
    };
}
