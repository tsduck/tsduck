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
#include "tsTLVSyntax.h"

namespace ts {
    //!
    //! Representation of MPEG PSI/SI tables in binary form (ie. list of sections).
    //!
    //! A table is built by adding sections using addSection().
    //! When all sections are present, the table becomes valid.
    //!
    //! Sections are added using @link SectionPtr @endlink safe pointers. Only the
    //! pointers are copied. The sections are shared.
    //!
    //! The @a table_id, @a version and number of sections is determined when
    //! the first section is added. Subsequent sections must have the
    //! same properties.
    //!
    class TSDUCKDLL BinaryTable
    {
    public:
        //!
        //! Default constructor.
        //!
        BinaryTable();

        //!
        //! Copy constructor.
        //! @param [in] table Another instance to copy.
        //! @param [in] mode The sections are either shared (ts::SHARE) between the
        //! two tables or duplicated (ts::COPY).
        //!
        BinaryTable(const BinaryTable& table, CopyShare mode);

        //!
        //! Constructor from an array of sections.
        //! @param [in] sections An array of smart pointers to sections.
        //! @param [in] replace If true, duplicated sections may be replaced.
        //! Otherwise, sections which are already present (based on section number)
        //! are not replaced.
        //! @param [in] grow If true, the "last_section_number" of a section
        //! may be greater than the current "last_section_number" of the table.
        //! In this case, all sections which were previously added in the table are modified.
        //!
        BinaryTable(const SectionPtrVector& sections, bool replace = true, bool grow = true);

        //!
        //! Assignment operator.
        //! The sections are referenced, and thus shared between the two table objects.
        //! @param [in] table Other table to assign to this object.
        //! @return A reference to this object.
        //!
        BinaryTable& operator=(const BinaryTable& table);

        //!
        //! Duplication.
        //! Similar to assignment but the sections are duplicated.
        //! @param [in] table Other table to duplicate into this object.
        //! @return A reference to this object.
        //!
        BinaryTable& copy(const BinaryTable& table);

        //!
        //! Equality operator.
        //! The source PID's are ignored, only the table contents are compared.
        //! Invalid tables are never identical.
        //! @param [in] table Other table to compare.
        //! @return True if the two tables are identical. False otherwise.
        //!
        bool operator==(const BinaryTable& table) const;

        //!
        //! Unequality operator.
        //! The source PID's are ignored, only the table contents are compared.
        //! Invalid tables are never identical.
        //! @param [in] table Other table to compare.
        //! @return True if the two tables are different. False otherwise.
        //!
        bool operator!=(const BinaryTable& table) const
        {
            return !(*this == table);
        }

        //!
        //! Add a section to a table.
        //! @param [in] section A smart pointers to section.
        //! @param [in] replace If true, duplicated sections may be replaced.
        //! Otherwise, sections which are already present (based on section number)
        //! are not replaced.
        //! @param [in] grow If true, the "last_section_number" of @a section
        //! may be greater than the current "last_section_number" of the table.
        //! In this case, all sections which were previously added in the table are modified.
        //! @return True on succes, false if @a section could not be added (inconsistent property).
        //!
        bool addSection(const SectionPtr& section, bool replace = true, bool grow = true);

        //!
        //! Add several sections to a table.
        //! @param [in] sections An array of smart pointers to sections.
        //! @param [in] replace If true, duplicated sections may be replaced.
        //! Otherwise, sections which are already present (based on section number)
        //! are not replaced.
        //! @param [in] grow If true, the "last_section_number" of a section
        //! may be greater than the current "last_section_number" of the table.
        //! In this case, all sections which were previously added in the table are modified.
        //! @return True on succes, false if a section could not be added (inconsistent property).
        //!
        bool addSections(const SectionPtrVector& sections, bool replace = true, bool grow = true);

        //!
        //! Check if the table is valid.
        //! @return True if the table is valid (all consistent sections are present with same
        //! table id, same version, same "last_section_number").
        //!
        bool isValid() const {return _is_valid;}

        //!
        //! Clear the content of the table.
        //! The table must be rebuilt using calls to addSection().
        //!
        void clear();

        //!
        //! Fast access to the table id.
        //! @return The table id.
        //!
        TID tableId() const {return _tid;}

        //!
        //! Fast access to the table id extension.
        //! @return The table id extension.
        //!
        uint16_t tableIdExtension() const {return _tid_ext;}

        //!
        //! Fast access to the table version number.
        //! @return The table version number.
        //!
        uint8_t version() const {return _version;}

        //!
        //! Fast access to the source PID.
        //! @return The source PID.
        //!
        PID sourcePID() const {return _source_pid;}

        //!
        //! Set the table id of all sections in the table.
        //! @param [in] tid The new table id.
        //! @param [in] recompute_crc If true, immediately recompute the CRC32 of all sections.
        //! If false, the CRC32's become invalid and must be computed later.
        //!
        void setTableIdExtension(uint16_t tid, bool recompute_crc = true);

        //!
        //! Set the table version number of all sections in the table.
        //! @param [in] version The new table version number.
        //! @param [in] recompute_crc If true, immediately recompute the CRC32 of all sections.
        //! If false, the CRC32's become invalid and must be computed later.
        //!
        void setVersion(uint8_t version, bool recompute_crc = true);

        //!
        //! Set the source PID of all sections in the table.
        //! @param [in] pid The new source PID.
        //!
        void setSourcePID(PID pid);

        //!
        //! Index of first TS packet of the table in the demultiplexed stream.
        //! Valid only if the table was extracted by a section demux.
        //! @return The first TS packet of the table in the demultiplexed stream.
        //!
        PacketCounter getFirstTSPacketIndex() const;

        //!
        //! Index of last TS packet of the table in the demultiplexed stream.
        //! Valid only if the table was extracted by a section demux.
        //! @return The last TS packet of the table in the demultiplexed stream.
        //!
        PacketCounter getLastTSPacketIndex() const;

        //!
        //! Number of sections in the table.
        //! @return The number of sections in the table.
        //!
        size_t sectionCount() const
        {
            return _sections.size();
        }

        //!
        //! Total size in bytes of all sections in the table.
        //! @return The total size in bytes of all sections in the table.
        //!
        size_t totalSize() const;

        //!
        //! Get a pointer to a section.
        //! @param [in] index Index of the section to get.
        //! @return A safe pointer to the section or a null pointer if the specified section is not present.
        //!
        const SectionPtr& sectionAt(size_t index) const
        {
            assert(index < _sections.size());
            return _sections[index];
        }

        //!
        //! Check if this is a table with one short section.
        //! @return True if this is a table with one short section.
        //!
        bool isShortSection() const;

        //!
        //! Write the binary table on a standard stream.
        //! @param [in,out] strm Output stream (binary output).
        //! @param [in,out] report Where to report errors.
        //! @return A reference to @a strm.
        //!
        std::ostream& write(std::ostream& strm, ReportInterface& report = CERR) const;

        //!
        //! Save the binary table in a file.
        //! @param [in] file_name Name of the output file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool save(const char* file_name, ReportInterface& report = CERR) const;

        //!
        //! Save the binary table in a file.
        //! @param [in] file_name Name of the output file.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool save(const std::string& file_name, ReportInterface& report = CERR) const
        {
            return save(file_name.c_str(), report);
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

    //!
    //! Safe pointer for BinaryTable (not thread-safe)
    //!
    typedef SafePtr<BinaryTable,NullMutex> BinaryTablePtr;

    //!
    //! Vector of BinaryTable pointers
    //!
    typedef std::vector<BinaryTablePtr> BinaryTablePtrVector;
}
