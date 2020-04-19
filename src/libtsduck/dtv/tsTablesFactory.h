//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
    class DVBCharTable;

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
        //! Get the list of standards which define a given table id.
        //! @param [in] id Table id.
        //! @return Corresponding list of standards.
        //!
        Standards getTableStandards(TID id) const;

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
        //! @param [in] nodeName Name of XML node.
        //! @return Corresponding factory or zero if there is none.
        //!
        TableFactory getTableFactory(const UString& nodeName) const;

        //!
        //! Get the descriptor factory for a given XML node name.
        //! @param [in] nodeName Name of XML node.
        //! @return Corresponding factory or zero if there is none.
        //!
        DescriptorFactory getDescriptorFactory(const UString& nodeName) const;

        //!
        //! Check if a descriptor is allowed in a table.
        //! @param [in] nodeName Name of the XML node for the descriptor.
        //! @param [in] table_id Table id of the table to check.
        //! @return True if the descriptor is allowed, false otherwise.
        //! Non-table-specific descriptors are allowed everywhere.
        //! Table-specific descriptors are allowed only in a set of specific tables.
        //!
        bool isDescriptorAllowed(const UString& nodeName, TID table_id) const;

        //!
        //! Get the list of tables where a descriptor is allowed.
        //! @param [in] nodeName Name of the XML node for the descriptor.
        //! @return Human-readable list of tables where the descriptor is allowed.
        //! Empty string for non-table-specific descriptors.
        //!
        UString descriptorTables(const UString& nodeName) const;

        //!
        //! Get the display function for a given table id.
        //! @param [in] id Table id.
        //! @param [in] cas Current CAS id.
        //! @return Corresponding display function or zero if there is none.
        //!
        DisplaySectionFunction getSectionDisplay(TID id, uint16_t cas = CASID_NULL) const;

        //!
        //! Get the log function for a given table id.
        //! @param [in] id Table id.
        //! @param [in] cas Current CAS id.
        //! @return Corresponding log function or zero if there is none.
        //!
        LogSectionFunction getSectionLog(TID id, uint16_t cas = CASID_NULL) const;

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
        //! Get the display function of the CA_descriptor for a given CA_system_id.
        //! @param [in] cas CA_system_id.
        //! @return Corresponding display function or zero if there is none.
        //!
        DisplayCADescriptorFunction getCADescriptorDisplay(uint16_t cas) const;

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
        //! Get the list of all registered additional XML model file names for tables and descriptors.
        //! @param [out] names List of all registered additional XML model file names.
        //!
        void getRegisteredTablesModels(UStringList& names) const;

        //!
        //! Get the list of all registered additional names files.
        //! @param [out] names List of all registered additional names files.
        //!
        void getRegisteredNamesFiles(UStringList& names) const;

        //!
        //! A class to register factories and display functions.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL Register
        {
            TS_NOBUILD_NOCOPY(Register);
        public:
            //!
            //! The constructor registers a table factory for a given id.
            //! @param [in] id Table id for this type.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //! @param [in] standards List of standards which define this table.
            //! @see TS_ID_TABLE_FACTORY
            //!
            Register(TID id, TableFactory factory, Standards standards);

            //!
            //! The constructor registers a table factory for a given range of ids.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //! @param [in] standards List of standards which define this table.
            //! @see TS_ID_TABLE_RANGE_FACTORY
            //!
            Register(TID minId, TID maxId, TableFactory factory, Standards standards);

            //!
            //! The constructor registers a descriptor factory for a given descriptor tag.
            //! @param [in] edid Exended descriptor id.
            //! @param [in] factory Function which creates a descriptor of the appropriate type.
            //! @see TS_ID_DESCRIPTOR_FACTORY
            //!
            Register(const EDID& edid, DescriptorFactory factory);

            //!
            //! The constructor registers a table factory for a given XML node name.
            //! @param [in] nodeName Name of XML nodes implementing this table.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //! @see TS_XML_TABLE_FACTORY
            //!
            Register(const UString& nodeName, TableFactory factory);

            //!
            //! The constructor registers a descriptor factory for a given XML node name.
            //! @param [in] nodeName Name of XML nodes implementing this descriptor.
            //! @param [in] factory Function which creates a descriptor of the appropriate type.
            //! @param [in] tids For table-specific descriptors, list of table ids where the descriptor is allowed to appear.
            //! @see TS_XML_DESCRIPTOR_FACTORY
            //!
            Register(const UString& nodeName, DescriptorFactory factory, std::initializer_list<TID> tids = std::initializer_list<TID>());

            //!
            //! The constructor registers a section display function for a given table id.
            //! @param [in] func Display function for the corresponding sections.
            //! @param [in] id Table id for this type.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(DisplaySectionFunction func, TID id, uint16_t minCAS = CASID_NULL, uint16_t maxCAS = CASID_NULL);

            //!
            //! The constructor registers a section display function for a given range of ids.
            //! @param [in] func Display function for the corresponding sections.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(DisplaySectionFunction func, TID minId, TID maxId, uint16_t minCAS, uint16_t maxCAS);

            //!
            //! The constructor registers a section log function for a given table id.
            //! @param [in] func Log function for the corresponding sections.
            //! @param [in] id Table id for this type.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(LogSectionFunction func, TID id, uint16_t minCAS = CASID_NULL, uint16_t maxCAS = CASID_NULL);

            //!
            //! The constructor registers a section log function for a given range of ids.
            //! @param [in] func Log function for the corresponding sections.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(LogSectionFunction func, TID minId, TID maxId, uint16_t minCAS, uint16_t maxCAS);

            //!
            //! The constructor registers a descriptor display function for a given descriptor id.
            //! @param [in] func Display function for the corresponding descriptors.
            //! @param [in] edid Exended descriptor id.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(DisplayDescriptorFunction func, const EDID& edid);

            //!
            //! The constructor registers a CA_descriptor display function for a given range of CA_system_id.
            //! @param [in] func Display function for the corresponding descriptors.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(DisplayCADescriptorFunction func, uint16_t minCAS, uint16_t maxCAS);
        };

        //!
        //! A class to register additional XML model files to merge with the main model for tables and descriptors.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL RegisterXML
        {
            TS_NOBUILD_NOCOPY(RegisterXML);
        public:
            //!
            //! Register an additional XML model file containing definitions for tables and descriptors.
            //! This file will be merged with the main model.
            //! @param [in] filename Name of the XML model file. This should be a simple file name,
            //! without directory. This file will be searched in the same directory as the executable,
            //! then in all directories from $TSPLUGINS_PATH, then from $LD_LIBRARY_PATH (Linux only),
            //! then from $PATH.
            //! @see TS_FACTORY_REGISTER_XML
            //!
            RegisterXML(const UString& filename);
        };


        //!
        //! A class to register additional names files to merge with the names file.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL RegisterNames
        {
            TS_NOBUILD_NOCOPY(RegisterNames);
        public:
            //!
            //! Register an additional names file.
            //! This file will be merged with the main names files.
            //! @param [in] filename Name of the names file. This should be a simple file name,
            //! without directory. This file will be searched in the same directory as the executable,
            //! then in all directories from $TSPLUGINS_PATH, then from $LD_LIBRARY_PATH (Linux only),
            //! then from $PATH.
            //! @see TS_FACTORY_REGISTER_NAMES
            //!
            RegisterNames(const UString& filename);
        };

    private:
        std::map<TID, TableFactory>                      _tableIds;
        std::map<TID, Standards>                         _tableStandards;
        std::map<EDID, DescriptorFactory>                _descriptorIds;
        std::map<UString, TableFactory>                  _tableNames;
        std::map<UString, DescriptorFactory>             _descriptorNames;
        std::multimap<UString, TID>                      _descriptorTablesIds;       // For table-specific descriptors
        std::map<uint32_t, DisplaySectionFunction>       _sectionDisplays;           // Key includes TID and CAS.
        std::map<uint32_t, LogSectionFunction>           _sectionLogs;               // Key includes TID and CAS.
        std::map<EDID, DisplayDescriptorFunction>        _descriptorDisplays;
        std::map<uint16_t, DisplayCADescriptorFunction>  _casIdDescriptorDisplays;   // Key is CAS system id.
        UStringList                                      _xmlModelFiles;             // Additional XML model files for tables.
        UStringList                                      _namesFiles;                // Additional names files.

        // Build a key in _sectionDisplays and _sectionLogs.
        static uint32_t SectionDisplayIndex(TID id, uint16_t cas);

        // Common code for getSectionDisplay and getSectionLog.
        template <typename FUNCTION>
        FUNCTION getSectionFunction(TID id, uint16_t cas, const std::map<uint32_t,FUNCTION>& funcMap) const;

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
#define _TS_FACTORY(rettype,classname)    namespace { rettype TS_UNIQUE_NAME(_Factory)() {return new classname;} }
#define _TS_TABLE_FACTORY(classname)      _TS_FACTORY(ts::AbstractTablePtr,classname)
#define _TS_DESCRIPTOR_FACTORY(classname) _TS_FACTORY(ts::AbstractDescriptorPtr,classname)
//! @endcond

//!
//! @hideinitializer
//! Registration inside the ts::TablesFactory singleton.
//! This macro is typically used in the .cpp file of a table or descriptor.
//!
#define TS_FACTORY_REGISTER static ts::TablesFactory::Register TS_UNIQUE_NAME(_Registrar)

//!
//! @hideinitializer
//! Registration of an extension XML model file inside the ts::TablesFactory singleton.
//! This macro is typically used in the .cpp file of a table or descriptor.
//!
#define TS_FACTORY_REGISTER_XML static ts::TablesFactory::RegisterXML TS_UNIQUE_NAME(_Registrar)

//!
//! @hideinitializer
//! Registration of an extension names file inside the ts::TablesFactory singleton.
//! This macro is typically used in the .cpp file of a table or descriptor.
//!
#define TS_FACTORY_REGISTER_NAMES static ts::TablesFactory::RegisterNames TS_UNIQUE_NAME(_Registrar)

//!
//! @hideinitializer
//! Registration of the table id of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_TABLE_FACTORY(classname,id,std) _TS_TABLE_FACTORY(classname) TS_FACTORY_REGISTER((id), TS_UNIQUE_NAME(_Factory), std)

//!
//! @hideinitializer
//! Registration of a range of table ids of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_TABLE_RANGE_FACTORY(classname,minId,maxId,std) _TS_TABLE_FACTORY(classname) TS_FACTORY_REGISTER((minId), (maxId), TS_UNIQUE_NAME(_Factory), std)

//!
//! @hideinitializer
//! Registration of the descriptor tag of a subclass of ts::AbstractDescriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_ID_DESCRIPTOR_FACTORY(classname,id) _TS_DESCRIPTOR_FACTORY(classname) TS_FACTORY_REGISTER((id), TS_UNIQUE_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_XML_TABLE_FACTORY(classname,xmlname) _TS_TABLE_FACTORY(classname) TS_FACTORY_REGISTER(xmlname, TS_UNIQUE_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractDescriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_XML_DESCRIPTOR_FACTORY(classname, xmlname) _TS_DESCRIPTOR_FACTORY(classname) TS_FACTORY_REGISTER(xmlname, TS_UNIQUE_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractDescriptor for a table-specific descriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_XML_TABSPEC_DESCRIPTOR_FACTORY(classname, xmlname, ...) _TS_DESCRIPTOR_FACTORY(classname) TS_FACTORY_REGISTER(xmlname, TS_UNIQUE_NAME(_Factory), {__VA_ARGS__})
