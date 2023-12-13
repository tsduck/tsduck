//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Binary or XML files containing PSI/SI sections and tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlJSONConverter.h"
#include "tsjson.h"
#include "tsTime.h"
#include "tsSection.h"
#include "tsBinaryTable.h"
#include "tsCRC32.h"
#include "tsBinaryTable.h"
#include "tsUString.h"
#include "tsEITOptions.h"
#include "tsxmlTweaks.h"
#include "tsTablesPtr.h"

namespace ts {
    //!
    //! A binary or XML file containing PSI/SI sections and tables.
    //! @ingroup mpeg
    //!
    //! A <i>section file</i> contains one or more sections. Short sections are
    //! also tables. Long sections need to be grouped to form a table. When a
    //! section file contains only complete valid tables, we also call it a
    //! <i>table file</i>.
    //!
    //! When a section file is loaded, the application can indifferently access:
    //!
    //! - All sections in the file.
    //! - All complete tables in the file.
    //! - Sections which do not belong to a table (<i>orphan sections</i>).
    //!
    //! There are currently two storage formats for section files: binary and XML.
    //! By default, file names ending in <code>.bin</code> are considered as binary files
    //! while names ending in <code>.xml</code> are considered as XML files.
    //! To manipulate other file formats, the application must specify the file type.
    //!
    //! Binary section file format
    //! --------------------------
    //!
    //! A binary section file is simply the concatenation of complete sections,
    //! header and payload, without any encapsulation. Sections must be read from
    //! the beginning of the file. The @e length field in the section header shall
    //! be used to locate the next section, immediately after the current section.
    //!
    //! Short sections are read and recognized as complete tables on their own.
    //! To get a valid table with long sections, all sections forming this table
    //! must be stored contiguously in the order of their section number.
    //!
    //! XML section file format
    //! -----------------------
    //!
    //! The format of XML section files is documented in the TSDuck user's guide.
    //! An informal template is given in file <code>tsduck.tables.model.xml</code>. This file
    //! is used to validate the content of XML section files.
    //!
    //! Sample XML section file:
    //! @code
    //! <?xml version="1.0" encoding="UTF-8"?>
    //! <tsduck>
    //!   <PAT version="8" current="true" transport_stream_id="0x0012" network_PID="0x0010">
    //!     <service service_id="0x0001" program_map_PID="0x1234"/>
    //!     <service service_id="0x0002" program_map_PID="0x0678"/>
    //!   </PAT>
    //! </tsduck>
    //! @endcode
    //!
    //! Each XML node describes a complete table. As a consequence, an XML section
    //! file contains complete tables only. There is no orphan section.
    //!
    class TSDUCKDLL SectionFile
    {
        TS_NOBUILD_NOCOPY(SectionFile);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //!
        SectionFile(DuckContext& duck);

        //!
        //! Section file formats.
        //!
        enum class FileType {
            UNSPECIFIED,  //!< Unspecified, depends on file name extension.
            BINARY,       //!< Binary section file.
            XML,          //!< XML section file.
            JSON,         //!< JSON (translated XML) section file.
        };

        //!
        //! Clear the list of loaded tables and sections.
        //!
        void clear();

        //!
        //! Get the size in bytes of all sections.
        //! This would be the size of the corresponding binary file.
        //! @return The size in bytes of all sections.
        //!
        size_t binarySize() const;

        //!
        //! Get the total number of sections in the file.
        //! @return The total number of sections in the file.
        //!
        size_t sectionsCount() const { return _sections.size(); }

        //!
        //! Get the total number of full tables in the file.
        //! Orphan sections are not included.
        //! @return The total number of full tables in the file.
        //!
        size_t tablesCount() const { return _tables.size(); }

        //!
        //! Get a file type, based on a file name.
        //! @param [in] file_name File name or inline XML or inline JSON.
        //! @param [in] type File type.
        //! @return If @a type is not FileType::UNSPECIFIED, return @a type.
        //! Otherwise, return the file type based on the file name. If the file
        //! name has no known extension, return FileType::UNSPECIFIED.
        //!
        static FileType GetFileType(const UString& file_name, FileType type = FileType::UNSPECIFIED);

        //!
        //! Build a file name, based on a file type.
        //! @param [in] file_name File name.
        //! @param [in] type File type.
        //! @return If @a type is not FileType::UNSPECIFIED, remove the
        //! extension from @a file_name and add the extension corresponding to @a type.
        //!
        static fs::path BuildFileName(const fs::path& file_name, FileType type);

        //!
        //! Set new parsing and formatting tweaks for XML files.
        //! @param [in] tweaks XML tweaks.
        //!
        void setTweaks(const xml::Tweaks& tweaks) { _xmlTweaks = tweaks; }

        //!
        //! Set the CRC32 processing mode when loading binary sections.
        //! @param [in] crc_op For binary files, how to process the CRC32 of the input sections.
        //!
        void setCRCValidation(CRC32::Validation crc_op) { _crc_op = crc_op; }

        //!
        //! Load a binary or XML file.
        //! The loaded sections are added to the content of this object.
        //! @param [in] file_name XML file name.
        //! If the file name starts with "<?xml", this is considered as "inline XML content".
        //! If the file name is empty or "-", the standard input is used.
        //! @param [in] type File type. If UNSPECIFIED, the file type is based on the file name.
        //! @return True on success, false on error.
        //!
        bool load(const UString& file_name, FileType type = FileType::UNSPECIFIED);

        //!
        //! Load a binary or XML file.
        //! The loaded sections are added to the content of this object.
        //! @param [in,out] strm A standard stream in input mode (binary mode for binary files).
        //! @param [in] type File type. If UNSPECIFIED, return an error.
        //! @return True on success, false on error.
        //!
        bool load(std::istream& strm, FileType type = FileType::UNSPECIFIED);

        //!
        //! Load an XML file.
        //! The loaded tables are added to the content of this object.
        //! @param [in] file_name XML file name.
        //! If the file name starts with "<?xml", this is considered as "inline XML content".
        //! If the file name is empty or "-", the standard input is used.
        //! @return True on success, false on error.
        //!
        bool loadXML(const UString& file_name);

        //!
        //! Load an XML file from an open text stream.
        //! The loaded sections are added to the content of this object.
        //! @param [in,out] strm A standard text stream in input mode.
        //! @return True on success, false on error.
        //!
        bool loadXML(std::istream& strm);

        //!
        //! Parse an XML content.
        //! The parsed tables are added to the content of this object.
        //! @param [in] xml_content XML file content in UTF-8.
        //! @return True on success, false on error.
        //!
        bool parseXML(const UString& xml_content);

        //!
        //! Load a JSON file.
        //! The JSON must have the format of a previous automated XML-to-JSON conversion.
        //! The loaded tables are added to the content of this object.
        //! @param [in] file_name JSON file name.
        //! If the file name starts with "{" or "[", this is considered as "inline JSON content".
        //! If the file name is empty or "-", the standard input is used.
        //! @return True on success, false on error.
        //!
        bool loadJSON(const UString& file_name);

        //!
        //! Load a JSON file from an open text stream.
        //! The JSON must have the format of a previous automated XML-to-JSON conversion.
        //! The loaded sections are added to the content of this object.
        //! @param [in,out] strm A standard text stream in input mode.
        //! @return True on success, false on error.
        //!
        bool loadJSON(std::istream& strm);

        //!
        //! Parse a JSON content.
        //! The JSON must have the format of a previous automated XML-to-JSON conversion.
        //! The parsed tables are added to the content of this object.
        //! @param [in] json_content JSON file content in UTF-8.
        //! @return True on success, false on error.
        //!
        bool parseJSON(const UString& json_content);

        //!
        //! Save an XML file.
        //! @param [in] file_name XML file name.
        //! If the file name is empty or "-", the standard output is used.
        //! @return True on success, false on error.
        //!
        bool saveXML(const UString& file_name) const;

        //!
        //! Save a JSON file after automated XML-to-JSON conversion.
        //! @param [in] file_name JSON file name.
        //! If the file name is empty or "-", the standard output is used.
        //! @return True on success, false on error.
        //!
        bool saveJSON(const UString& file_name);

        //!
        //! Serialize as XML text.
        //! @return Complete XML document text, empty on error.
        //!
        UString toXML() const;

        //!
        //! Serialize as JSON text.
        //! @return Complete JSON document text, empty on error.
        //!
        UString toJSON();

        //!
        //! Load a binary section file from a stream.
        //! The loaded sections are added to the content of this object.
        //! @param [in,out] strm A standard stream in input mode (binary mode).
        //! @return True on success, false on error.
        //!
        bool loadBinary(std::istream& strm)
        {
            return loadBinary(strm, _report);
        }

        //!
        //! Load a binary section file.
        //! The loaded sections are added to the content of this object.
        //! @param [in] file_name Binary file name.
        //! If the file name is empty or "-", the standard input is used.
        //! @return True on success, false on error.
        //!
        bool loadBinary(const fs::path& file_name);

        //!
        //! Save a binary section file.
        //! @param [in,out] strm A standard stream in output mode (binary mode).
        //! @return True on success, false on error.
        //!
        bool saveBinary(std::ostream& strm) const
        {
            return saveBinary(strm, _report);
        }

        //!
        //! Save a binary section file.
        //! @param [in] file_name Binary file name.
        //! If the file name is empty or "-", the standard output is used.
        //! @return True on success, false on error.
        //!
        bool saveBinary(const fs::path& file_name) const;

        //!
        //! Load a binary section file from a memory buffer.
        //! The loaded sections are added to the content of this object.
        //! @param [in] data Address of the memory buffer.
        //! @param [in] size Size in bytes of the memory buffer.
        //! @return True on success, false if some sections were incorrect or truncated.
        //!
        bool loadBuffer(const void* data, size_t size);

        //!
        //! Load a binary section file from a memory buffer.
        //! The loaded sections are added to the content of this object.
        //! @param [in] data Byte block containing the binary data.
        //! @param [in] start Starting index inside the byte buffer.
        //! @param [in] count Size in bytes to read after @a start. If set to NPOS, use the rest of the buffer.
        //! @return True on success, false if some sections were incorrect or truncated.
        //!
        bool loadBuffer(const ByteBlock& data, size_t start = 0, size_t count = NPOS);

        //!
        //! Save the section file into a memory buffer.
        //! @param [out] buffer Address of the memory buffer to write.
        //! @param [in] buffer_size Size in bytes of the memory buffer.
        //! @return The written size in bytes. If the buffer is too short, no section is truncated.
        //! The returned size includes complete sections only. Use binarySize() to get the required total size.
        //!
        size_t saveBuffer(void* buffer, size_t buffer_size) const;

        //!
        //! Save the section file into a memory buffer.
        //! @param [in,out] buffer A byte block into which the sections are written.
        //! The sections are appended to the existing content of @a buffer.
        //! @return The number of bytes which were appended to the buffer.
        //!
        size_t saveBuffer(ByteBlock& buffer) const;

        //!
        //! Fast access to the list of loaded tables.
        //! @return A constant reference to the internal list of loaded tables.
        //!
        const BinaryTablePtrVector& tables() const
        {
            return _tables;
        }

        //!
        //! Fast access to the list of loaded sections.
        //! @return A constant reference to the internal list of loaded sections.
        //!
        const SectionPtrVector& sections() const
        {
            return _sections;
        }

        //!
        //! Fast access to the list of orphan sections, sections which are not part of a table.
        //! @return A constant reference to the internal list of orphan sections.
        //!
        const SectionPtrVector& orphanSections() const
        {
            return _orphanSections;
        }

        //!
        //! Get a copy of the list of loaded tables.
        //! @param [out] tables The list of loaded tables.
        //!
        void getTables(BinaryTablePtrVector& tables) const
        {
            tables.assign(_tables.begin(), _tables.end());
        }

        //!
        //! Get a copy of the list of loaded sections.
        //! @param [out] sections The list of loaded sections.
        //!
        void getSections(SectionPtrVector& sections) const
        {
            sections.assign(_sections.begin(), _sections.end());
        }

        //!
        //! Get a copy of the list of orphan sections.
        //! @param [out] sections The list of orphan sections.
        //!
        void getOrphanSections(SectionPtrVector& sections) const
        {
            sections.assign(_orphanSections.begin(), _orphanSections.end());
        }

        //!
        //! Add a binary table in the file.
        //! If the table is not complete (there are missing sections),
        //! the sections which are present are individually added.
        //! @param [in] table The binary table to add.
        //!
        void add(const BinaryTablePtr& table);

        //!
        //! Add several binary tables in the file.
        //! If a table is not complete (there are missing sections),
        //! the sections which are present are individually added.
        //! @param [in] tables The binary tables to add.
        //!
        void add(const BinaryTablePtrVector& tables);

        //!
        //! Add a typed table in the file.
        //! The table is serialized first. Then its sections are added in the file.
        //! @param [in] table The table to add.
        //!
        void add(const AbstractTablePtr& table);

        //!
        //! Add a section in the file.
        //! @param [in] section The binary section to add.
        //!
        void add(const SectionPtr& section);

        //!
        //! Add several sections in the file.
        //! @param [in] sections The binary sections to add.
        //!
        void add(const SectionPtrVector& sections);

        //!
        //! Pack all orphan sections.
        //! Consecutive sections from the same tables are packed: the sections are
        //! renumbered starting at zero. The result is a complete but potentially
        //! invalid section.
        //! @return The number of tables which were created.
        //!
        size_t packOrphanSections();

        //!
        //! Reorganize all EIT sections according to ETSI TS 101 211.
        //!
        //! Only one EITp/f subtable is kept per service. It is split in two sections if two
        //! events (present and following) are specified. All EIT schedule are kept. But they
        //! are completely reorganized. All events are extracted and spread over new EIT
        //! sections according to ETSI TS 101 211 rules.
        //!
        //! @param [in] reftime Reference time for EIT schedule. Only the date part is used.
        //! This is the "last midnight" according to which EIT segments are assigned. By
        //! default, the oldest event start time is used.
        //! @param [in] options Generation options for EIT (p/f and/or schedule, actual and/or other).
        //! @see ts::EIT::ReorganizeSections()
        //! @see ETSI TS 101 211, section 4.1.4
        //!
        void reorganizeEITs(const Time& reftime = Time(), EITOptions options = EITOptions::GEN_ALL);

        //!
        //! This static method loads the XML model for tables and descriptors.
        //! It loads the main model and merges all extensions.
        //! @param [out] doc XML document which receives the model.
        //! @param [out] load_extensions If true (the default), load model additions from all declared TSDuck extensions.
        //! @return True on success, false on error.
        //!
        static bool LoadModel(xml::Document& doc, bool load_extensions = true);

        //!
        //! Default file name suffix for binary section files.
        //!
        static constexpr const UChar* const DEFAULT_BINARY_SECTION_FILE_SUFFIX = u".bin";

        //!
        //! Default file name suffix for XML section files.
        //!
        static constexpr const UChar* const DEFAULT_XML_SECTION_FILE_SUFFIX = u".xml";

        //!
        //! Default file name suffix for JSON section files.
        //!
        static constexpr const UChar* const DEFAULT_JSON_SECTION_FILE_SUFFIX = u".json";

        //!
        //! File name of the XML model file for tables.
        //!
        static constexpr const UChar* const XML_TABLES_MODEL = u"tsduck.tables.model.xml";

    private:
        DuckContext&         _duck;                   // Reference to TSDuck execution context.
        Report&              _report;                 // Where to report errors.
        BinaryTablePtrVector _tables {};              // Loaded tables.
        SectionPtrVector     _sections {};            // All sections from the file.
        SectionPtrVector     _orphanSections {};      // Sections which do not belong to any table.
        xml::JSONConverter   _model {_report};        // XML model for tables.
        xml::Tweaks          _xmlTweaks {};           // XML formatting and parsing tweaks.
        CRC32::Validation    _crc_op = CRC32::IGNORE; // Processing of CRC32 when loading sections.

        // Load the XML model in this instance, if not already done.
        bool loadThisModel();

        // Load/save a binary section file from a stream with specific report.
        bool loadBinary(std::istream& strm, Report& report);
        bool saveBinary(std::ostream& strm, Report& report) const;

        // Rebuild _tables and _orphanSections from _sections.
        void rebuildTables();

        // Parse an XML document.
        bool parseDocument(const xml::Document& doc);

        // Generate an XML document.
        bool generateDocument(xml::Document& doc) const;

        // Check it a table can be formed using the last sections in _orphanSections.
        void collectLastTable();

        // Generate a JSON document. Point to a JSON Null literal on error.
        json::ValuePtr convertToJSON();
    };
}
