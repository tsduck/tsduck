//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of MPEG PES packets
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsByteBlock.h"
#include "tsMPEG.h"

namespace ts {

    class PESPacket;

    // Safe pointer for PESPacket (not thread-safe)
    typedef SafePtr <PESPacket, NullMutex> PESPacketPtr;

    // Vector of PESPacket pointers
    typedef std::vector <PESPacketPtr> PESPacketPtrVector;

    class TSDUCKDLL PESPacket
    {
    public:
        // Default constructor. PESPacket is initially marked invalid.
        PESPacket()
        {
            initialize (PID_NULL);
        }

        // Copy constructor. The packet content is either shared or copied.
        PESPacket (const PESPacket&, CopyShare);

        // Constructor from full binary content.
        // The content is copied into the packet if valid.
        PESPacket (const void* content, size_t content_size, PID source_pid = PID_NULL)
        {
            initialize (new ByteBlock (content, content_size), source_pid);
        }

        // Constructor from full binary content.
        // The content is copied into the packet if valid.
        PESPacket (const ByteBlock& content, PID source_pid = PID_NULL)
        {
            initialize (new ByteBlock (content), source_pid);
        }

        // Constructor from full binary content.
        // The content is referenced, and thus shared.
        // Do not modify the referenced ByteBlock from outside the PESPacket.
        PESPacket (const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL)
        {
            initialize (content_ptr, source_pid);
        }

        // Reload full binary content.
        // The content is copied into the packet if valid.
        void reload (const void* content, size_t content_size, PID source_pid = PID_NULL)
        {
            initialize (new ByteBlock (content, content_size), source_pid);
        }

        // Reload full binary content.
        // The content is copied into the packet if valid.
        void reload (const ByteBlock& content, PID source_pid = PID_NULL)
        {
            initialize (new ByteBlock (content), source_pid);
        }

        // Reload full binary content.
        // The content is referenced, and thus shared.
        // Do not modify the referenced ByteBlock from outside the PESPacket.
        void reload (const ByteBlockPtr& content_ptr, PID source_pid = PID_NULL)
        {
            initialize (content_ptr, source_pid);
        }

        // Clear packet content. Becomes invalid packets.
        void clear()
        {
            _is_valid = false;
            _header_size = 0;
            _source_pid = PID_NULL;
            _data.clear();
        }

        // Assignment. The packet content is referenced, and thus shared
        // between the two packet objects.
        PESPacket& operator= (const PESPacket&);

        // Duplication. Similar to assignment but the content of the packet
        // is duplicated.
        PESPacket& copy (const PESPacket&);

        // Check if a packet has valid content
        bool isValid() const {return _is_valid;}

        // Comparison.
        // The source PID are ignored, only the packet contents are compared.
        // Note: Invalid packets are never identical
        bool operator== (const PESPacket&) const;
        bool operator!= (const PESPacket& pp) const {return !(*this == pp);}

        // PID from which the packet was collected
        PID getSourcePID() const {return _source_pid;}
        void setSourcePID (PID pid) {_source_pid = pid;}

        // Index of first and last TS packet of the PES packet in the demultiplexed stream
        PacketCounter getFirstTSPacketIndex() const {return _first_pkt;}
        PacketCounter getLastTSPacketIndex () const {return _last_pkt;}
        void setFirstTSPacketIndex (PacketCounter i) {_first_pkt = i;}
        void setLastTSPacketIndex  (PacketCounter i) {_last_pkt = i;}

        // Stream id
        uint8_t getStreamId() const {return _is_valid ? (*_data)[3] : 0;}
        void setStreamId (uint8_t sid) {if (_is_valid) (*_data)[3] = sid;}

        // Check if the packet has a long header
        bool hasLongHeader() const {return _is_valid && IsLongHeaderSID ((*_data)[3]);}

        // Access to the full binary content of the packet.
        // Do not modify content.
        // May be invalidated after modification in packet.
        const uint8_t* content() const {return _data->data();}
        size_t size() const {return _data->size();}

        // PES header and payload
        const uint8_t* header() const {return _is_valid ? _data->data() : 0;}
        size_t headerSize() const {return _is_valid ? _header_size : 0;}
        const uint8_t* payload() const {return _is_valid ? _data->data() + _header_size : 0;}
        size_t payloadSize() const {return _is_valid ? _data->size() - _header_size : 0;}

        // Check if the PES packet contains MPEG-2 video (also applies to MPEG-1 video)
        bool isMPEG2Video() const;

        // Check if the PES packet contains AVC.
        bool isAVC() const;

        // Check if the PES packet contains AC-3 or Enhanced-AC-3.
        // Warning: As specified in ETSI TS 102 366, an AC-3 audio frame always
        // starts with 0x0B77. This is what we check here. However, it is still
        // possible that other encodings may start from time to time with 0x0B77.
        // Thus, it is safe to say that a PID in which all PES packets start with
        // 0x0B77 (ie isAC3() returns true) contains AC-3. However, if only
        // a few PES packets start with 0x0B77, it is safe to say that it should be
        // something else.
        bool isAC3() const;

    private:
        // Private fields
        bool          _is_valid;     // Content of *_data is a valid packet
        size_t        _header_size;  // PES header size in bytes
        PID           _source_pid;   // Source PID (informational)
        PacketCounter _first_pkt;    // Index of first packet in stream
        PacketCounter _last_pkt;     // Index of last packet in stream
        ByteBlockPtr  _data;         // Full binary content of the packet

        // Helpers for constructors
        void initialize(PID);
        void initialize(const ByteBlockPtr&, PID);

        // Inaccessible operations
        PESPacket(const PESPacket&) = delete;
    };
}
