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
//!  Binary or XML files containing PSI/SI sections and tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsxmlDocument.h"
#include "tsxmlElement.h"
#include "tsMPEG.h"
#include "tsUString.h"
#include "tsDVBCharset.h"
#include "tsTablesPtr.h"

//!
//! Default suffix of binary section file names.
//!
#define TS_DEFAULT_BINARY_SECTION_FILE_SUFFIX u".bin"
//!
//! Default suffix of XML section file names.
//!
#define TS_DEFAULT_XML_SECTION_FILE_SUFFIX u".xml"

namespace ts {
    //!
    //! A binary or XML file containing PSI/SI sections and tables.
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
    //! ### Binary section file format
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
    //! ### XML section file format
    //!
    //! The format of XML section files is documented in the TSDuck user's guide.
    //! An informal template is given in file <code>tsduck.xml</code>. This file
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
    public:
        //!
        //! Default constructor.
        //!
        SectionFile();

        //!
        //! Destructor.
        //!
        virtual ~SectionFile();

        //!
        //! Section file formats.
        //!
        enum FileType {
            UNSPECIFIED,  //!< Unspecified, depends on file name extension.
            BINARY,       //!< Binary section file.
            XML,          //!< XML section file.
        };

        //!
        //! Clear the list of loaded tables and sections.
        //!
        void clear();

        //!
        //! Load an XML file.
        //! @param [in] file_name XML file name.
        //! @param [in,out] report Where to report errors.
        //! @param [in] charset If not zero, default character set to encode strings.
        //! @return True on success, false on error.
        //!
        bool loadXML(const UString& file_name, Report& report, const DVBCharset* charset = 0);

        //!
        //! Parse an XML content.
        //! @param [in] xml_content XML file content in UTF-8.
        //! @param [in,out] report Where to report errors.
        //! @param [in] charset If not zero, default character set to encode strings.
        //! @return True on success, false on error.
        //!
        bool parseXML(const UString& xml_content, Report& report, const DVBCharset* charset = 0);

        //!
        //! Save an XML file.
        //! @param [in] file_name XML file name.
        //! @param [in,out] report Where to report errors.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //! @return True on success, false on error.
        //!
        bool saveXML(const UString& file_name, Report& report, const DVBCharset* charset = 0) const;

        //!
        //! Serialize as XML text.
        //! @param [in,out] report Where to report errors.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //! @return Complete XML document text, empty on error.
        //!
        UString toText(Report& report, const DVBCharset* charset = 0) const;

        //!
        //! Fast access to the list of loaded tables.
        //! @return A constant reference to the internal list of loaded tables.
        //!
        const BinaryTablePtrVector& tables() const
        {
            return _tables;
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
        //! Add a table in the file.
        //! @param [in] table The binary table to add.
        //!
        void add(const BinaryTablePtr& table);

        //!
        //! Add several tables in the file.
        //! @param [in] tables The binary tables to add.
        //!
        void add(const BinaryTablePtrVector& tables);

        //!
        //! Add a table in the file.
        //! The table is serialized
        //! @param [in] table The table to add.
        //! @param [in] charset If not zero, default character set to encode strings.
        //!
        void add(const AbstractTablePtr& table, const DVBCharset* charset = 0);

    private:
        BinaryTablePtrVector _tables;          //!< Loaded tables.
        SectionPtrVector     _sections;        //!< All sections from the file.
        SectionPtrVector     _orphanSections;  //!< Sections which do not belong to any table.

        //!
        //! Parse an XML document.
        //! @param [in] doc Document to load.
        //! @param [in] charset If not zero, default character set to encode strings.
        //! @return True on success, false on error.
        //!
        bool parseDocument(const xml::Document& doc, const DVBCharset* charset);

        //!
        //! Generate an XML document.
        //! @param [in,out] doc XML document.
        //! @param [in] charset If not zero, character set to use without explicit table code.
        //! @return True on success, false on error.
        //!
        bool generateDocument(xml::Document& doc, const DVBCharset* charset) const;

        //!
        //! Check it a table can be formed using the last sections in _sections.
        //! If the sections were present at end of _orphanSections, they are removed.
        //!
        void collectLastTable();
    };
}
