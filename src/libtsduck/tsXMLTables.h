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
#include "tsAbstractTable.h"
#include "tsAbstractDescriptor.h"
#include "tsSingletonManager.h"

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
        bool load(const std::string& file_name, ReportInterface& report);

        //!
        //! Save an XML file.
        //! @param [in] file_name XML file name.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool save(const std::string& file_name, ReportInterface& report) const;

        //!
        //! Parse an XML content.
        //! @param [in] xml_content XML file content.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error.
        //!
        bool parse(const std::string& xml_content, ReportInterface& report);

        //!
        //! Serialize as XML text.
        //! @param [in,out] report Where to report errors.
        //! @return XML text, empty on error.
        //!
        std::string toText(ReportInterface& report) const;

        //!
        //! Fast access to the list of loaded tables.
        //! @return A constant reference to the internal list of loaded tables.
        //!
        const AbstractTablePtrVector& tables() const
        {
            return _tables;
        }

        //!
        //! Get a copy of the list of loaded tables.
        //! @param [out] tables The list of loaded tables.
        //!
        void getTables(AbstractTablePtrVector& tables) const
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
        //! @param [in] table The table to add.
        //!
        void add(const AbstractTablePtr& table)
        {
            _tables.push_back(table);
        }

        //!
        //! Get the list of all registered XML names for tables.
        //! @param [out] names List of all registered XML names for tables.
        //!
        static void GetRegisteredTableNames(StringList& names);

        //!
        //! Get the list of all registered XML names for descriptors.
        //! @param [out] names List of all registered XML names for descriptors.
        //!
        static void GetRegisteredDescriptorNames(StringList& names);

        //!
        //! Profile of a function which creates a table.
        //!
        typedef AbstractTablePtr (*TableFactory)();

        //!
        //! Profile of a function which creates a descriptor.
        //!
        typedef AbstractDescriptorPtr (*DescriptorFactory)();

        //!
        //! Register a table factory for a given XML node name.
        //!
        class RegisterTableType
        {
        public:
            //!
            //! The constructor registers a table factory for a given XML node name.
            //! @param [in] node_name Name of XML nodes implementing this table.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //!
            RegisterTableType(const std::string& node_name, TableFactory factory);

        private:
            RegisterTableType() = delete;
            RegisterTableType(const RegisterTableType&) = delete;
            RegisterTableType& operator=(const RegisterTableType&) = delete;
        };

        //!
        //! Register a descriptor factory for a given XML node name.
        //!
        class RegisterDescriptorType
        {
        public:
            //!
            //! The constructor registers a descriptor factory for a given XML node name.
            //! @param [in] node_name Name of XML nodes implementing this descriptor.
            //! @param [in] factory Function which creates a descriptor of the appropriate type.
            //!
            RegisterDescriptorType(const std::string& node_name, DescriptorFactory factory);

        private:
            RegisterDescriptorType() = delete;
            RegisterDescriptorType(const RegisterDescriptorType&) = delete;
            RegisterDescriptorType& operator=(const RegisterDescriptorType&) = delete;
        };

    private:
        AbstractTablePtrVector _tables;   //!< Loaded tables.

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

        //!
        //! A singleton class which tracks all table factories.
        //!
        class TSDUCKDLL TableFactories : public std::map<std::string, TableFactory>
        {
            tsDeclareSingleton(TableFactories);
        };

        //!
        //! A singleton class which tracks all descriptor factories.
        //!
        class TSDUCKDLL DescriptorFactories : public std::map<std::string, DescriptorFactory>
        {
            tsDeclareSingleton(DescriptorFactories);
        };
    };
}

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_XML_TABLE_FACTORY(classname, xmlname)      \
    namespace {                                       \
        ts::AbstractTablePtr _XMLFactory_##xmlname()  \
        {                                             \
            return new classname;                     \
        }                                             \
    }                                                 \
    static ts::XMLTables::RegisterTableType _XMLRegister_##xmlname(#xmlname, _XMLFactory_##xmlname)

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractDescriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_XML_DESCRIPTOR_FACTORY(classname, xmlname)      \
    namespace {                                            \
        ts::AbstractDescriptorPtr _XMLFactory_##xmlname()  \
        {                                                  \
            return new classname;                          \
        }                                                  \
    }                                                      \
    static ts::XMLTables::RegisterDescriptorType _XMLRegister_##xmlname(#xmlname, _XMLFactory_##xmlname)
