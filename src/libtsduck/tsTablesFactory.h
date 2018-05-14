//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  Tables and descriptor factory.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsMPEG.h"
#include "tsEDID.h"
#include "tsSection.h"
#include "tsTablesPtr.h"
#include "tsSingletonManager.h"

namespace ts {

    class TablesDisplay;
    class DVBCharset;

    //!
    //! A factory class which creates tables and descriptors based on id or name.
    //!
    //! This class is a singleton. Use static Instance() method to access the single instance.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TablesFactory
    {
        TS_DECLARE_SINGLETON(TablesFactory);

    public:
        //!
        //! Profile of a function which creates a table.
        //! @return A safe pointer to an instance of a concrete subclass of AbstractTable.
        //!
        typedef AbstractTablePtr (*TableFactory)();

        //!
        //! Profile of a function which creates a descriptor.
        //! @return A safe pointer to an instance of a concrete subclass of AbstractDescriptor.
        //!
        typedef AbstractDescriptorPtr (*DescriptorFactory)();

        //!
        //! Get the table factory for a given table id.
        //! @param [in] id Table id.
        //! @return Corresponding factory or zero if there is none.
        //!
        TableFactory getTableFactory(TID id) const;

        //!
        //! Get the descriptor factory for a given descriptor tag.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] tid Optional table id of the table containing the descriptor.
        //! If @a edid is a standard descriptor and @a tid is specified, try first a
        //! table-specific descriptor for this table. Fallback to the standard descriptor.
        //! @return Corresponding factory or zero if there is none.
        //!
        DescriptorFactory getDescriptorFactory(const EDID& edid, TID tid = TID_NULL) const;

        //!
        //! Get the table factory for a given XML node name.
        //! @param [in] node_name Name of XML node.
        //! @return Corresponding factory or zero if there is none.
        //!
        TableFactory getTableFactory(const UString& node_name) const;

        //!
        //! Get the descriptor factory for a given XML node name.
        //! @param [in] node_name Name of XML node.
        //! @return Corresponding factory or zero if there is none.
        //!
        DescriptorFactory getDescriptorFactory(const UString& node_name) const;

        //!
        //! Check if a descriptor is allowed in a table.
        //! @param [in] desc_node_name Name of the XML node for the descriptor.
        //! @param [in] table_id Table id of the table to check.
        //! @return True if the descriptor is allowed, false otherwise.
        //! Non-table-specific descriptors are allowed everywhere.
        //! Table-specific descriptors are allowed only in a set of specific tables.
        //!
        bool isDescriptorAllowed(const UString& desc_node_name, TID table_id) const;

        //!
        //! Get the list of tables where a descriptor is allowed.
        //! @param [in] desc_node_name Name of the XML node for the descriptor.
        //! @return Human-readable list of tables where the descriptor is allowed.
        //! Empty string for non-table-specific descriptors.
        //!
        UString descriptorTables(const UString& desc_node_name) const;

        //!
        //! Get the display function for a given table id.
        //! @param [in] id Table id.
        //! @return Corresponding display function or zero if there is none.
        //!
        DisplaySectionFunction getSectionDisplay(TID id) const;

        //!
        //! Get the display function for a given extended descriptor id.
        //! @param [in] edid Extended descriptor id.
        //! @param [in] tid Optional table id of the table containing the descriptor.
        //! If @a edid is a standard descriptor and @a tid is specified, try first a
        //! table-specific descriptor for this table. Fallback to the standard descriptor.
        //! @return Corresponding display function or zero if there is none.
        //!
        DisplayDescriptorFunction getDescriptorDisplay(const EDID& edid, TID tid = TID_NULL) const;

        //!
        //! Get the list of all registered table ids.
        //! @param [out] ids List of all registered table ids.
        //!
        void getRegisteredTableIds(std::vector<TID>& ids) const;

        //!
        //! Get the list of all registered descriptor tags.
        //! @param [out] ids List of all registered descriptor tags.
        //!
        void getRegisteredDescriptorIds(std::vector<EDID>& ids) const;

        //!
        //! Get the list of all registered XML names for tables.
        //! @param [out] names List of all registered XML names for tables.
        //!
        void getRegisteredTableNames(UStringList& names) const;

        //!
        //! Get the list of all registered XML names for descriptors.
        //! @param [out] names List of all registered XML names for descriptors.
        //!
        void getRegisteredDescriptorNames(UStringList& names) const;

        //!
        //! A class to register factories and display functions.
        //!
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class Register
        {
        public:
            //!
            //! The constructor registers a table factory for a given id.
            //! @param [in] id Table id for this type.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //! @see TS_ID_TABLE_FACTORY
            //!
            Register(TID id, TableFactory factory);

            //!
            //! The constructor registers a table factory for a given range of ids.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //! @see TS_ID_TABLE_RANGE_FACTORY
            //!
            Register(TID minId, TID maxId, TableFactory factory);

            //!
            //! The constructor registers a descriptor factory for a given descriptor tag.
            //! @param [in] edid Exended descriptor id.
            //! @param [in] factory Function which creates a descriptor of the appropriate type.
            //! @see TS_ID_DESCRIPTOR_FACTORY
            //!
            Register(const EDID& edid, DescriptorFactory factory);

            //!
            //! The constructor registers a table factory for a given XML node name.
            //! @param [in] node_name Name of XML nodes implementing this table.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //! @see TS_XML_TABLE_FACTORY
            //!
            Register(const UString& node_name, TableFactory factory);

            //!
            //! The constructor registers a descriptor factory for a given XML node name.
            //! @param [in] node_name Name of XML nodes implementing this descriptor.
            //! @param [in] factory Function which creates a descriptor of the appropriate type.
            //! @param [in] tids For table-specific descriptors, list of table ids where the descriptor is allowed to appear.
            //! @see TS_XML_DESCRIPTOR_FACTORY
            //!
            Register(const UString& node_name, DescriptorFactory factory, std::initializer_list<TID> tids = std::initializer_list<TID>());

            //!
            //! The constructor registers a section display function for a given table id.
            //! @param [in] id Table id for this type.
            //! @param [in] func Display function for the corresponding sections.
            //! @see TS_ID_SECTION_DISPLAY
            //!
            Register(TID id, DisplaySectionFunction func);

            //!
            //! The constructor registers a section display function for a given range of ids.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] func Display function for the corresponding sections.
            //! @see TS_ID_SECTION_RANGE_DISPLAY
            //!
            Register(TID minId, TID maxId, DisplaySectionFunction func);

            //!
            //! The constructor registers a descriptor display function for a given descriptor id.
            //! @param [in] edid Exended descriptor id.
            //! @param [in] func Display function for the corresponding descriptors.
            //! @see TS_ID_DESCRIPTOR_DISPLAY
            //!
            Register(const EDID& edid, DisplayDescriptorFunction func);

        private:
            // Inaccessible operations.
            Register() = delete;
            Register(const Register&) = delete;
            Register& operator=(const Register&) = delete;
        };

    private:
        std::map<TID, TableFactory>               _tableIds;
        std::map<EDID, DescriptorFactory>         _descriptorIds;
        std::map<UString, TableFactory>           _tableNames;
        std::map<UString, DescriptorFactory>      _descriptorNames;
        std::multimap<UString, TID>               _descriptorTablesIds;  // For table-specific descriptors
        std::map<TID, DisplaySectionFunction>     _sectionDisplays;
        std::map<EDID, DisplayDescriptorFunction> _descriptorDisplays;

        // Common code for getDescriptorFactory and getDescriptorDisplay.
        template <typename FUNCTION>
        FUNCTION getDescriptorFunction(const EDID& edid, TID tid, const std::map<EDID,FUNCTION>& funcMap) const;
    };
}

//
// Implementation note: Take care before modifying the following macros.
// Especially, the *_FACTORY macros need to be defined on one single line
// each because of the use of __LINE__ to create unique identifiers.
//

//! @cond nodoxygen
#define _TS_FACTORY_NAME1(a,b) a##b
#define _TS_FACTORY_NAME2(a,b) _TS_FACTORY_NAME1(a,b)
#define _TS_FACTORY_NAME(a)    _TS_FACTORY_NAME2(a,__LINE__)

#define _TS_FACTORY(rettype,classname)    namespace { rettype _TS_FACTORY_NAME(_Factory)() {return new classname;} }
#define _TS_TABLE_FACTORY(classname)      _TS_FACTORY(ts::AbstractTablePtr,classname)
#define _TS_DESCRIPTOR_FACTORY(classname) _TS_FACTORY(ts::AbstractDescriptorPtr,classname)
#define _TS_FACTORY_REGISTER              static ts::TablesFactory::Register _TS_FACTORY_NAME(_Registrar)
//! @endcond

//!
//! @hideinitializer
//! Registration of the table id of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_TABLE_FACTORY(classname,id) _TS_TABLE_FACTORY(classname) _TS_FACTORY_REGISTER((id), _TS_FACTORY_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of a range of table ids of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_TABLE_RANGE_FACTORY(classname,minId,maxId) _TS_TABLE_FACTORY(classname) _TS_FACTORY_REGISTER((minId), (maxId), _TS_FACTORY_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the descriptor tag of a subclass of ts::AbstractDescriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_ID_DESCRIPTOR_FACTORY(classname,id) _TS_DESCRIPTOR_FACTORY(classname) _TS_FACTORY_REGISTER((id), _TS_FACTORY_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_XML_TABLE_FACTORY(classname,xmlname) _TS_TABLE_FACTORY(classname) _TS_FACTORY_REGISTER(xmlname, _TS_FACTORY_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractDescriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_XML_DESCRIPTOR_FACTORY(classname, xmlname) _TS_DESCRIPTOR_FACTORY(classname) _TS_FACTORY_REGISTER(xmlname, _TS_FACTORY_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractDescriptor for a table-specific descriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_XML_TABSPEC_DESCRIPTOR_FACTORY(classname, xmlname, ...) _TS_DESCRIPTOR_FACTORY(classname) _TS_FACTORY_REGISTER(xmlname, _TS_FACTORY_NAME(_Factory), {__VA_ARGS__})

//!
//! @hideinitializer
//! Registration of the display function for a table id.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_SECTION_DISPLAY(func, id) _TS_FACTORY_REGISTER((id), (func))

//!
//! @hideinitializer
//! Registration of the display function for a range of table ids.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_SECTION_RANGE_DISPLAY(func, minId, maxId) _TS_FACTORY_REGISTER((minId), (maxId), (func))

//!
//! @hideinitializer
//! Registration of the display function for a descriptor id.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_ID_DESCRIPTOR_DISPLAY(func, edid) _TS_FACTORY_REGISTER((edid), (func))
