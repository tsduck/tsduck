//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of MPEG PSI/SI tables in binary form (ie. list of sections)
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDefinedByStandards.h"
#include "tsTablesPtr.h"
#include "tsTS.h"
#include "tsxml.h"

namespace ts {

    class DuckContext;

    //!
    //! Representation of MPEG PSI/SI tables in binary form (ie. list of sections).
    //! @ingroup mpeg
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
    class TSDUCKDLL BinaryTable : public AbstractDefinedByStandards
    {
    public:
        //!
        //! Default constructor.
        //!
        BinaryTable() = default;

        //!
        //! Copy constructor.
        //! @param [in] table Another instance to copy.
        //! @param [in] mode The sections are either shared (ShareMode::SHARE) between the
        //! two tables or duplicated (ShareMode::COPY).
        //!
        BinaryTable(const BinaryTable& table, ShareMode mode);

        //!
        //! Move constructor.
        //! @param [in,out] table Another instance to move.
        //!
        BinaryTable(BinaryTable&& table) noexcept;

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
        //! Move assignment operator.
        //! The sections are referenced, and thus shared between the two table objects.
        //! @param [in,out] table Other table to move into this object.
        //! @return A reference to this object.
        //!
        BinaryTable& operator=(BinaryTable&& table) noexcept;

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
        TS_UNEQUAL_OPERATOR(BinaryTable)

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
        bool addSections(const SectionPtrVector& sections, bool replace = true, bool grow = true)
        {
            return addSections(sections.begin(), sections.end(), replace, grow);
        }

        //!
        //! Add several sections to a table.
        //! @param [in] first First iterator to an array of smart pointers to sections.
        //! @param [in] last Last iterator to an array of smart pointers to sections.
        //! @param [in] replace If true, duplicated sections may be replaced.
        //! Otherwise, sections which are already present (based on section number)
        //! are not replaced.
        //! @param [in] grow If true, the "last_section_number" of a section
        //! may be greater than the current "last_section_number" of the table.
        //! In this case, all sections which were previously added in the table are modified.
        //! @return True on succes, false if a section could not be added (inconsistent property).
        //!
        bool addSections(SectionPtrVector::const_iterator first, SectionPtrVector::const_iterator last, bool replace = true, bool grow = true);

        //!
        //! Pack all sections in a table, removing references to missing sections.
        //! As an example, if a table expects 5 sections (numbered 0 to 4) but only
        //! sections numbered 1 and 3 are present, the table is packed with the
        //! two existing sections renumbered 0 and 1 and the last section number
        //! is set to 2 in the existing sections. The table becomes valid if at least
        //! one section was present.
        //! @return True on succes, false if the table is empty.
        //!
        bool packSections();

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
        PacketCounter firstTSPacketIndex() const;

        //!
        //! Index of last TS packet of the table in the demultiplexed stream.
        //! Valid only if the table was extracted by a section demux.
        //! @return The last TS packet of the table in the demultiplexed stream.
        //!
        PacketCounter lastTSPacketIndex() const;

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
        //! Minimum number of TS packets required to transport the table.
        //! @param [in] pack If true, assume that sections are packed in TS packets.
        //! When false, assume that each section starts at the beginning of a TS packet
        //! and stuffing in applied at the end of each section.
        //! @return The minimum number of TS packets required to transport the table.
        //!
        PacketCounter packetCount(bool pack = true) const;

        //!
        //! Get a pointer to a section.
        //! @param [in] index Index of the section to get.
        //! @return A safe pointer to the section or a null pointer if the specified section is not present.
        //!
        const SectionPtr sectionAt(size_t index) const;

        //!
        //! Check if this is a table with one short section.
        //! @return True if this is a table with one short section.
        //!
        bool isShortSection() const;

        //!
        //! Options to convert a binary table into XML.
        //!
        class TSDUCKDLL XMLOptions
        {
        public:
            XMLOptions();       //!< Constructor.
            bool forceGeneric;  //!< Force a generic table node even if the table can be specialized.
            bool setPID;        //!< Add a metadata element with the source PID, when available.
            bool setLocalTime;  //!< Add a metadata element with the current local time.
            bool setPackets;    //!< Add a metadata element with the index of the first and last TS packets of the table.
        };

        //!
        //! This method converts the table to XML.
        //! If the table has a specialized implementation, generate a specialized XML structure.
        //! Otherwise, generate a \<generic_short_table> or \<generic_long_table> node.
        //! @param [in,out] duck TSDuck execution environment.
        //! @param [in,out] parent The parent node for the XML representation.
        //! @param [in] opt Conversion options.
        //! @return The new XML element or zero if the table is not valid.
        //!
        xml::Element* toXML(DuckContext& duck, xml::Element* parent, const XMLOptions& opt = XMLOptions()) const;

        //!
        //! This method converts an XML node as a binary table.
        //! @param [in,out] duck TSDuck execution environment.
        //! @param [in] node The root of the XML descriptor.
        //! @return True if the XML element name is a valid table name, false otherwise.
        //! If the name is valid but the content is incorrect, true is returned and this object is invalidated.
        //!
        bool fromXML(DuckContext& duck, const xml::Element* node);

        // Implementation of AbstractDefinedByStandards
        virtual Standards definingStandards() const override;

    private:
        BinaryTable(const BinaryTable& table) = delete;

        // Private fields
        bool             _is_valid = false;
        TID              _tid {TID_NULL};
        uint16_t         _tid_ext = 0;
        uint8_t          _version = 0;
        PID              _source_pid = PID_NULL;
        int              _missing_count = 0;
        SectionPtrVector _sections {};
    };
}
