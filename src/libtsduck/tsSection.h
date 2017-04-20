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
//!  Representation of MPEG PSI/SI sections
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsCerrReport.h"
#include "tsByteBlock.h"
#include "tsSafePtr.h"
#include "tsCASFamily.h"
#include "tsCRC32.h"
#include "tsETID.h"

namespace ts {

    class Section;

    // Safe pointer for Section (not thread-safe)
    typedef SafePtr <Section, NullMutex> SectionPtr;

    // Vector of Section pointers
    typedef std::vector <SectionPtr> SectionPtrVector;

    // General note
    //
    // What to do with the CRC32 when building a section depends on the
    // parameter named crc_op:
    //
    //   IGNORE:  Neither check nor compute
    //   CHECK:   Validate the CRC from the section data. Mark section as
    //            invalid if CRC is incorrect.
    //   COMPUTE: Compute the CRC and store it in the section.
    //
    // Typically, if the ByteBlock comes from the wire, use CHECK.
    // If the ByteBlock is built by the application, use COMPUTE,

    class TSDUCKDLL Section
    {
    public:
        // Default constructor. Section is initially marked invalid.
        Section()
        {
            initialize (PID_NULL);
        }

        // Copy constructor. The section content is either shared or copied.
        Section (const Section& sect, CopyShare mode);

        // Constructor from full binary content.
        // The content is copied into the section if valid.
        Section (const void* content,
                 size_t content_size,
                 PID source_pid = PID_NULL,
                 CRC32::Validation crc_op = CRC32::IGNORE)
        {
            initialize (new ByteBlock (content, content_size), source_pid, crc_op);
        }

        // Constructor from full binary content.
        // The content is copied into the section if valid.
        Section (const ByteBlock& content,
                 PID source_pid = PID_NULL,
                 CRC32::Validation crc_op = CRC32::IGNORE)
        {
            initialize (new ByteBlock (content), source_pid, crc_op);
        }

        // Constructor from full binary content.
        // The content is referenced, and thus shared.
        // Do not modify the referenced ByteBlock from outside the Section.
        Section (const ByteBlockPtr& content_ptr,
                 PID source_pid = PID_NULL,
                 CRC32::Validation crc_op = CRC32::IGNORE)
        {
            initialize (content_ptr, source_pid, crc_op);
        }

        // Constructor from short section payload.
        Section (TID tid,
                 bool is_private_section,
                 const void* payload,
                 size_t payload_size,
                 PID source_pid = PID_NULL)
        {
            reload (tid, is_private_section, payload, payload_size, source_pid);
        }

        // Constructor from long section payload.
        // The provided payload does not contain the CRC32.
        // The CRC32 is automatically computed.
        Section (TID tid,
                 bool is_private_section,
                 uint16_t tid_ext,
                 uint8_t version,
                 bool is_current,
                 uint8_t section_number,
                 uint8_t last_section_number,
                 const void* payload,
                 size_t payload_size,
                 PID source_pid = PID_NULL)
        {
            reload (tid, is_private_section, tid_ext, version, is_current,
                    section_number, last_section_number,
                    payload, payload_size, source_pid);
        }

        // Reload full binary content.
        // The content is copied into the section if valid.
        void reload (const void* content,
                     size_t content_size,
                     PID source_pid = PID_NULL,
                     CRC32::Validation crc_op = CRC32::IGNORE)
        {
            initialize (new ByteBlock (content, content_size), source_pid, crc_op);
        }

        // Reload full binary content.
        // The content is copied into the section if valid.
        void reload (const ByteBlock& content,
                     PID source_pid = PID_NULL,
                     CRC32::Validation crc_op = CRC32::IGNORE)
        {
            initialize (new ByteBlock (content), source_pid, crc_op);
        }

        // Reload full binary content.
        // The content is referenced, and thus shared.
        // Do not modify the referenced ByteBlock from outside the Section.
        void reload (const ByteBlockPtr& content_ptr,
                     PID source_pid = PID_NULL,
                     CRC32::Validation crc_op = CRC32::IGNORE)
        {
            initialize (content_ptr, source_pid, crc_op);
        }

        // Reload short section
        void reload (TID tid,
                     bool is_private_section,
                     const void* payload,
                     size_t payload_size,
                     PID source_pid = PID_NULL);

        // Reload long section.
        // The provided payload does not contain the CRC32.
        // The CRC32 is automatically computed.
        void reload (TID tid,
                     bool is_private_section,
                     uint16_t tid_ext,
                     uint8_t version,
                     bool is_current,
                     uint8_t section_number,
                     uint8_t last_section_number,
                     const void* payload,
                     size_t payload_size,
                     PID source_pid = PID_NULL);

        // Clear section content. Becomes invalid sections.
        void clear()
        {
            _is_valid = false;
            _source_pid = PID_NULL;
            _first_pkt = 0;
            _last_pkt = 0;
            _data.clear();
        }

        // Assignment. The section content is referenced, and thus shared
        // between the two section objects.
        Section& operator= (const Section& sect);

        // Duplication. Similar to assignment but the content of the section
        // is duplicated.
        Section& copy (const Section& sect);

        // Check if a section has valid content
        bool isValid() const {return _is_valid;}

        // Comparison.
        // The source PID are ignored, only the section contents are compared.
        // Note: Invalid sections are never identical
        bool operator== (const Section& sect) const;
        bool operator!= (const Section& sect) const {return !(*this == sect);}

        // Common section properties
        TID tableId() const
        {
            return _is_valid ? (*_data)[0] : 0xFF;
        }
        bool isLongSection() const
        {
            return _is_valid ? ((*_data)[1] & 0x80) != 0 : false;
        }
        bool isShortSection() const {
            return _is_valid ? ((*_data)[1] & 0x80) == 0 : false;
        }
        bool isPrivateSection() const
        {
            return _is_valid ? ((*_data)[1] & 0x40) != 0 : false;
        }

        // Long section properties
        uint16_t tableIdExtension() const
        {
            return isLongSection() ? GetUInt16 (&(*_data)[3]) : 0;
        }
        uint8_t version() const
        {
            return isLongSection() ? (((*_data)[5] >> 1) & 0x1F) : 0;
        }
        bool isCurrent() const
        {
            return isLongSection() ? ((*_data)[5] & 0x01) != 0 : false;
        }
        bool isNext() const
        {
            return isLongSection() ? ((*_data)[5] & 0x01) == 0 : false;
        }
        uint8_t sectionNumber() const
        {
            return isLongSection() ? (*_data)[6] : 0;
        }
        uint8_t lastSectionNumber() const
        {
            return isLongSection() ? (*_data)[7] : 0;
        }

        // Extended TID
        ETID etid() const
        {
            return isLongSection() ? ETID (tableId(), tableIdExtension()) : ETID (tableId());
        }

        // Return the PID from which the section was collected
        PID sourcePID() const
        {
            return _source_pid;
        }

        // Access to the full binary content of the section.
        // Do not modify content.
        // May be invalidated after modification in section.
        const uint8_t* content() const {return _data->data();}
        size_t size() const {return _data->size();}

        // Size of the section header.
        size_t headerSize() const
        {
            return _is_valid ? (isLongSection() ? LONG_SECTION_HEADER_SIZE : SHORT_SECTION_HEADER_SIZE) : 0;
        }

        // Access to the payload of the section.
        // For short sections, the payload starts after the
        // private_section_length field. For long sections, the payload
        // starts after the last_section_number field and ends before
        // the CRC32 field. Do not modify payload content.
        // May be invalidated after modification in section.
        const uint8_t* payload() const
        {
            return _is_valid ? _data->data() + (isLongSection() ? LONG_SECTION_HEADER_SIZE : SHORT_SECTION_HEADER_SIZE) : 0;
        }
        size_t payloadSize() const
        {
            return _is_valid ? _data->size() - (isLongSection() ? LONG_SECTION_HEADER_SIZE + SECTION_CRC32_SIZE : SHORT_SECTION_HEADER_SIZE) : 0;
        }

        // Minimum number of TS packets required to transport the section.
        PacketCounter packetCount() const {return SectionPacketCount (size());}

        // Modifiable properties.
        void setTableIdExtension (uint16_t, bool recompute_crc = true);
        void setVersion (uint8_t, bool recompute_crc = true);
        void setIsCurrent (bool, bool recompute_crc = true);
        void setSectionNumber (uint8_t, bool recompute_crc = true);
        void setLastSectionNumber (uint8_t, bool recompute_crc = true);
        void setSourcePID (PID pid) {_source_pid = pid;}

        // Index of first and last TS packet of the section in the demultiplexed stream
        PacketCounter getFirstTSPacketIndex() const {return _first_pkt;}
        PacketCounter getLastTSPacketIndex () const {return _last_pkt;}
        void setFirstTSPacketIndex (PacketCounter i) {_first_pkt = i;}
        void setLastTSPacketIndex  (PacketCounter i) {_last_pkt = i;}

        // This method recomputes and replaces the CRC32 of the section.
        void recomputeCRC();

        // Read and write section on standard streams.
        // On input, if a section is invalid (eof before end of section,
        // wrong crc), the failbit of the stream is set.
        std::istream& read (std::istream& strm, CRC32::Validation crc_op = CRC32::IGNORE, ReportInterface& report = CERR);
        std::ostream& write (std::ostream& strm, ReportInterface& report = CERR) const;

        // Display the section on an output stream with full interpretation
        std::ostream& display (std::ostream& strm, int indent = 0, CASFamily cas = CAS_OTHER, bool no_header = false) const;

        // Hexa dump the section on an output stream without interpretation of the payload.
        std::ostream& dump (std::ostream& strm, int indent = 0, CASFamily cas = CAS_OTHER, bool no_header = false) const;

        // These static methods read all sections from the specified file.
        // Return true on success, false on error.
        static bool LoadFile (SectionPtrVector& sections,
                              std::istream& strm,
                              CRC32::Validation crc_op = CRC32::IGNORE,
                              ReportInterface& report = CERR);

        static bool LoadFile (SectionPtrVector& sections,
                              const std::string& file_name,
                              CRC32::Validation crc_op = CRC32::IGNORE,
                              ReportInterface& report = CERR);

    private:
        // Private fields
        bool          _is_valid;    // Content of *_data is a valid section
        PID           _source_pid;  // Source PID (informational)
        PacketCounter _first_pkt;   // Index of first packet in stream (informational)
        PacketCounter _last_pkt;    // Index of last packet in stream (informational)
        ByteBlockPtr  _data;        // Full binary content of the section

        // Helpers for constructors
        void initialize (PID);
        void initialize (const ByteBlockPtr&, PID, CRC32::Validation);

        // Inaccessible operations
        Section(const Section&) = delete;
    };
}

// Display operator for sections
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::Section& sect)
{
    return sect.display (strm);
}
