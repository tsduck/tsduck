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
//!  Representation of MPEG PSI/SI tables in binary form (ie. list of sections)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsSection.h"

namespace ts {

    // A table is built by adding sections using addSection().
    // When all sections are present, the table becomes valid.
    //
    // Sections are added using SectionPtr safe pointers. Only the
    // pointers are copied. The sections are shared.
    //
    // The table_id, version and number of sections is determined when
    // the first section is added. Subsequent sections must have the
    // same properties.

    class TSDUCKDLL BinaryTable
    {
    public:
        // Constructor
        BinaryTable ();

        // Copy constructor. The sections are either shared between the
        // two tables or duplicated.
        BinaryTable(const BinaryTable& table, CopyShare mode);

        // Constructor from an array of sections.
        // If "replace" is true, duplicated sections may be replaced.
        // If "grow" is true, the "last_section_number" of the section
        // may be greater than the current "last_section_number".
        // In this case, all sections which were previously added
        // in the table are modified.
        BinaryTable(const SectionPtrVector& sections, bool replace = true, bool grow = true);

        // Assignment. The sections are referenced, and thus shared
        // between the two table objects.
        BinaryTable& operator=(const BinaryTable& table);

        // Duplication. Similar to assignment but the sections are duplicated.
        BinaryTable& copy(const BinaryTable& table);

        // Comparison.
        // The source PID are ignored, only the table contents are compared.
        // Note: Invalid tables are never identical
        bool operator==(const BinaryTable& table) const;
        bool operator!=(const BinaryTable& table) const {return !(*this == table);}

        // Add a section to a table.
        // If "replace" is true, an existing section may be replaced.
        //
        // If "grow" is true, the "last_section_number" of the section
        // may be greater than the current "last_section_number".
        // In this case, all sections which were previously added
        // in the table are modified.
        //
        // AddSection returns false if the section could
        // not be added (inconsistent property).
        bool addSection(const SectionPtr& section, bool replace = true, bool grow = true);

        // Add several sections to a table
        bool addSections(const SectionPtrVector& sections, bool replace = true, bool grow = true);

        // Check if the table is valid.
        bool isValid() const {return _is_valid;}

        // Clear the content of the table. The table must be rebuilt
        // using calls to addSection.
        void clear();

        // Fast access to main table properties.
        // Other fields must be explicitely access through the sections.
        TID tableId() const {return _tid;}
        uint16_t tableIdExtension() const {return _tid_ext;}
        uint8_t version() const {return _version;}
        PID sourcePID() const {return _source_pid;}

        // Modifiable properties.
        void setTableIdExtension(uint16_t, bool recompute_crc = true);
        void setVersion(uint8_t, bool recompute_crc = true);
        void setSourcePID(PID);

        // Index of first and last TS packet of the table in the demultiplexed stream.
        PacketCounter getFirstTSPacketIndex() const;
        PacketCounter getLastTSPacketIndex() const;

        // Return the number of sections in the table
        size_t sectionCount() const {return _sections.size ();}

        // Return the total size in bytes of all sections in the table.
        size_t totalSize() const;

        // Get a pointer to a section.
        const SectionPtr& sectionAt (size_t index) const
        {
            assert (index < _sections.size ());
            return _sections[index];
        }

        // Display the table on an output stream
        std::ostream& display (std::ostream& strm, int indent = 0, CASFamily cas = CAS_OTHER) const;

        // Write the binary table on standard streams.
        std::ostream& write (std::ostream& strm, ReportInterface& report = CERR) const;

        // Save the binary table in a file. Return true on success, false on error.
        bool save (const char* file_name, ReportInterface& report = CERR) const;
        bool save (const std::string& file_name, ReportInterface& report = CERR) const
        {
            return save (file_name.c_str(), report);
        }

    private:
        BinaryTable(const BinaryTable& table) = delete;

        // Private fields
        bool             _is_valid;
        TID              _tid;
        uint16_t         _tid_ext;
        uint8_t          _version;
        PID              _source_pid;
        int              _missing_count;
        SectionPtrVector _sections;
    };

    // Safe pointer for BinaryTable (not thread-safe)
    typedef SafePtr<BinaryTable,NullMutex> BinaryTablePtr;

    // Vector of BinaryTable pointers
    typedef std::vector<BinaryTablePtr> BinaryTablePtrVector;
}

// Display operator for tables
TSDUCKDLL inline std::ostream& operator<< (std::ostream& strm, const ts::BinaryTable& table)
{
    return table.display (strm);
}
