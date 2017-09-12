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
//!  XML files containing PSI/SI tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsXML.h"
#include "tsMPEG.h"
#include "tsTablesPtr.h"
#include "tsStringUtils.h"

namespace ts {
    //!
    //! An XML file containing PSI/SI tables.
    //!
    class TSDUCKDLL XMLTables
    {
    public:
        //!
        //! Default constructor.
        //!
        XMLTables();

        //!
        //! Load an XML file.
        //! @param [in] file_name XML file name.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool loadXML(const std::string& file_name, ReportInterface& report);

        //!
        //! Parse an XML content.
        //! @param [in] xml_content XML file content.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool parseXML(const std::string& xml_content, ReportInterface& report);

        //!
        //! Save an XML file.
        //! @param [in] file_name XML file name.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool saveXML(const std::string& file_name, ReportInterface& report) const;

        //!
        //! Serialize as XML text.
        //! @param [in,out] report Where to report errors.
        //! @return Complete XML document text, empty on error.
        //!
        std::string toText(ReportInterface& report) const;

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
        //! Clear the list of loaded tables.
        //!
        void clear()
        {
            _tables.clear();
        }

        //!
        //! Add a table in the file.
        //! @param [in] table The binary table to add.
        //!
        void add(const BinaryTablePtr& table)
        {
            _tables.push_back(table);
        }

        //!
        //! Add several tables in the file.
        //! @param [in] tables The binary tables to add.
        //!
        void add(const BinaryTablePtrVector& tables)
        {
            _tables.insert(_tables.end(), tables.begin(), tables.end());
        }

        //!
        //! Add a table in the file.
        //! The table is serialized
        //! @param [in] table The table to add.
        //!
        void add(const AbstractTablePtr& table);

        //--------------------------------------------------------------------
        // PSI/SI to XML utilities.
        //--------------------------------------------------------------------

        //!
        //! This method converts a table to the appropriate XML tree.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @param [in] table The table to serialize.
        //! @return The new XML element or zero if @a table is invalid.
        //!
        static XML::Element* ToXML(XML& xml, XML::Element* parent, const BinaryTable& table);

        //!
        //! This method converts a descriptor to the appropriate XML tree.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @param [in] desc The descriptor to serialize.
        //! @param [in] pds Associated private data specifier.
        //! @return The new XML element or zero if @a table is invalid.
        //!
        static XML::Element* ToXML(XML& xml, XML::Element* parent, const Descriptor& desc, PDS pds = 0);

        //!
        //! This method converts a list of descriptors to XML.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the all descriptors.
        //! @param [in] list The list of descriptors to serialize.
        //! @return True on success, false on error.
        //!
        static bool ToXML(XML& xml, XML::Element* parent, const DescriptorList& list);

        //!
        //! This method converts a generic table to XML.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @param [in] table The table to serialize.
        //! @return The new XML element or zero if @a table is invalid.
        //!
        static XML::Element* ToGenericTable(XML& xml, XML::Element* parent, const BinaryTable& table);

        //!
        //! This method converts a generic descriptor to XML.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in,out] parent The parent node for the new XML tree.
        //! @param [in] desc The descriptor to serialize.
        //! @return The new XML element or zero if @a desc is invalid.
        //!
        static XML::Element* ToGenericDescriptor(XML& xml, XML::Element* parent, const Descriptor& desc);

        //--------------------------------------------------------------------
        // XML to PSI/SI utilities.
        //--------------------------------------------------------------------

        //!
        //! This method decodes an XML list of descriptors.
        //! @param [out] list Returned descriptor list.
        //! @param [out] others Returned list of non-descriptor XML elements.
        //! All these elements are not null and their names are in @a allowedOthers.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] parent The XML element containing all descriptors.
        //! @param [in] allowedOthers A list of allowed element names inside @a parent
        //! which are not descriptors.
        //! @return True on success, false on error.
        //!
        static bool FromDescriptorListXML(DescriptorList& list, XML::ElementVector& others, XML& xml, const XML::Element* parent, const StringList& allowedOthers);

        //!
        //! This method decodes an XML list of descriptors.
        //! @param [out] list Returned descriptor list.
        //! @param [out] others Returned list of non-descriptor XML elements.
        //! All these elements are not null and their names are in @a allowedOthers.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] parent The XML element containing all descriptors.
        //! @param [in] allowedOthers A comma-separated list of allowed element names inside @a parent
        //! which are not descriptors.
        //! @return True on success, false on error.
        //!
        static bool FromDescriptorListXML(DescriptorList& list, XML::ElementVector& others, XML& xml, const XML::Element* parent, const std::string& allowedOthers)
        {
            StringList allowed;
            return FromDescriptorListXML(list, others, xml, parent, SplitString(allowed, allowedOthers));
        }

        //!
        //! This method decodes an XML list of descriptors.
        //! @param [out] list Returned descriptor list.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] parent The XML element containing all descriptors.
        //! All children must be valid descriptors.
        //! @return True on success, false on error.
        //!
        static bool FromDescriptorListXML(DescriptorList& list, XML& xml, const XML::Element* parent)
        {
            XML::ElementVector others;
            return FromDescriptorListXML(list, others, xml, parent, StringList());
        }

        //!
        //! This method decodes a \<generic_short_table> or \<generic_long_table>.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] elem The XML element.
        //! @return A safe pointer to the decoded table or a null pointer on error.
        //!
        static BinaryTablePtr FromGenericTableXML(XML& xml, const XML::Element* elem);

        //!
        //! This method decodes a \<generic_descriptor>.
        //! @param [in,out] xml XML utility for error reporting
        //! @param [in] elem The XML element.
        //! @return A safe pointer to the decoded descriptor or a null pointer on error.
        //!
        static DescriptorPtr FromGenericDescriptorXML(XML& xml, const XML::Element* elem);

    private:
        BinaryTablePtrVector _tables;   //!< Loaded tables.

        //!
        //! Parse an XML document.
        //! @param [in,out] xml XML handling.
        //! @param [in] doc Document to load.
        //! @return True on success, false on error.
        //!
        bool parseDocument(XML& xml, const XML::Document& doc);

        //!
        //! Generate an XML document.
        //! @param [in,out] xml XML handling.
        //! @param [in,out] printer XML printer.
        //! @return True on success, false on error.
        //!
        bool generateDocument(XML& xml, XML::Printer& printer) const;
    };
}
