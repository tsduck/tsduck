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

    class DuckContext;
    class TablesDisplay;
    class DVBCharTable;

    //!
    //! A factory class which creates tables and descriptors based on id or name.
    //!
    //! This class is a singleton. Use static Instance() method to access the single instance.
    //!
    //! Multi-threading considerations: The singleton is built and modified using static
    //! registration instances during the initialization of the application (ie. in one
    //! single thread). Then, the singleton is only read during the execution of the
    //! application. So, no explicit synchronization is required.
    //!
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
        //! @param [in] standards List of current active standards in the application.
        //! If there are several factories for this table id, return only a factory for
        //! which the standard is active. For instance, if the same table id is used by
        //! ATSC and ISDB but the application runs in an ISDB context, return the factory
        //! for the ISDB version of this table id.
        //! @param [in] pid PID on which the section is found.
        //! @param [in] cas Current CAS id.
        //! @return Corresponding factory or zero if there is none.
        //!
        TableFactory getTableFactory(TID id, Standards standards, PID pid = PID_NULL, uint16_t cas = CASID_NULL) const;

        //!
        //! Get the list of standards which are defined for a given table id.
        //! @param [in] id Table id.
        //! @return Corresponding list of standards. If multiple definitions exist for this
        //! table id, return the common subset of all definitions. This means that only
        //! standards which are used in all cases are returned.
        //! @param [in] pid PID on which the section is found.
        //!
        Standards getTableStandards(TID id, PID pid = PID_NULL) const;

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
        //! @param [in] tid Table id of the table to check.
        //! @return True if the descriptor is allowed, false otherwise.
        //! Non-table-specific descriptors are allowed everywhere.
        //! Table-specific descriptors are allowed only in a set of specific tables.
        //!
        bool isDescriptorAllowed(const UString& nodeName, TID tid) const;

        //!
        //! Get the list of tables where a descriptor is allowed.
        //! @param [in] duck TSDuck execution context to interpret table names.
        //! @param [in] nodeName Name of the XML node for the descriptor.
        //! @return Human-readable list of tables where the descriptor is allowed.
        //! Empty string for non-table-specific descriptors.
        //!
        UString descriptorTables(const DuckContext& duck, const UString& nodeName) const;

        //!
        //! Get the display function for a given table id.
        //! @param [in] id Table id.
        //! @param [in] standards List of current active standards in the application.
        //! If there are several display functions for this table id, return only a
        //! function for which the standard is active. For instance, if the same table
        //! id is used by ATSC and ISDB but the application runs in an ISDB context,
        //! return the display function for the ISDB version of this table id.
        //! @param [in] pid PID on which the section is found.
        //! @param [in] cas Current CAS id.
        //! @return Corresponding display function or zero if there is none.
        //!
        DisplaySectionFunction getSectionDisplay(TID id, Standards standards, PID pid = PID_NULL, uint16_t cas = CASID_NULL) const;

        //!
        //! Get the log function for a given table id.
        //! @param [in] id Table id.
        //! @param [in] standards List of current active standards in the application.
        //! If there are several log functions for this table id, return only a
        //! function for which the standard is active. For instance, if the same table
        //! id is used by ATSC and ISDB but the application runs in an ISDB context,
        //! return the log function for the ISDB version of this table id.
        //! @param [in] pid PID on which the section is found.
        //! @param [in] cas Current CAS id.
        //! @return Corresponding log function or zero if there is none.
        //!
        LogSectionFunction getSectionLog(TID id, Standards standards, PID pid = PID_NULL, uint16_t cas = CASID_NULL) const;

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
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! The PID can be used to resolve conflicting table ids between standards. For instance,
            //! a table id 0xC7 on PID 0x0025 is interpreted as an ISDB-defined LDT while the same
            //! table id on PID 0x1FFB is interpreted as an ATSC-defined MGT.
            //! @see TS_ID_TABLE_FACTORY
            //! @see TS_ID_TABLE_PIDS_FACTORY
            //!
            Register(TID id, TableFactory factory, Standards standards, std::initializer_list<PID> pids = std::initializer_list<PID>());

            //!
            //! The constructor registers a table factory for a given range of ids.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] factory Function which creates a table of the appropriate type.
            //! @param [in] standards List of standards which define this table.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @see TS_ID_TABLE_RANGE_FACTORY
            //!
            Register(TID minId, TID maxId, TableFactory factory, Standards standards, std::initializer_list<PID> pids = std::initializer_list<PID>());

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
            //! @param [in] standards List of standards which define this table.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(DisplaySectionFunction func, TID id, Standards standards, uint16_t minCAS = CASID_NULL, uint16_t maxCAS = CASID_NULL, std::initializer_list<PID> pids = std::initializer_list<PID>());

            //!
            //! The constructor registers a section display function for a given range of ids.
            //! @param [in] func Display function for the corresponding sections.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] standards List of standards which define this table.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(DisplaySectionFunction func, TID minId, TID maxId, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids = std::initializer_list<PID>());

            //!
            //! The constructor registers a section log function for a given table id.
            //! @param [in] func Log function for the corresponding sections.
            //! @param [in] id Table id for this type.
            //! @param [in] standards List of standards which define this table.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(LogSectionFunction func, TID id, Standards standards, uint16_t minCAS = CASID_NULL, uint16_t maxCAS = CASID_NULL, std::initializer_list<PID> pids = std::initializer_list<PID>());

            //!
            //! The constructor registers a section log function for a given range of ids.
            //! @param [in] func Log function for the corresponding sections.
            //! @param [in] minId Minimum table id for this type.
            //! @param [in] maxId Maximum table id for this type.
            //! @param [in] standards List of standards which define this table.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @see TS_FACTORY_REGISTER
            //!
            Register(LogSectionFunction func, TID minId, TID maxId, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids = std::initializer_list<PID>());

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
        // Description of a table id. Several descriptions can be used for the same table id,
        // for instance for distinct DTV standards or disctinct CA systems.
        // We use a fixed-size array for 'pids' instead of a PIDSet for storage efficiency.
        class TableDescription
        {
        public:
            Standards              standards;  // Standards for this table id.
            uint16_t               minCAS;     // Minimum CAS id for this table id (CASID_NULL if none).
            uint16_t               maxCAS;     // Maximum CAS id for this table id (CASID_NULL if none).
            TableFactory           factory;    // Function to build an instance of the table.
            DisplaySectionFunction display;    // Function to display a section.
            LogSectionFunction     log;        // Function to log a section.
            std::array<PID,8>      pids;       // Standard PID's for the standard, stop at first PID_NULL.

            // Constructor.
            TableDescription();

            // Add PIDs in the list.
            void addPIDs(std::initializer_list<PID> morePIDs);

            // Check if a PID is present.
            bool hasPID(PID pid) const;
        };

        std::multimap<TID, TableDescription>             _tables;                   // Description of all table ids, potential multiple entries per table idx
        std::map<EDID, DescriptorFactory>                _descriptorIds;            // Extended descriptor id to descriptor factory
        std::map<UString, TableFactory>                  _tableNames;               // XML table name to table factory
        std::map<UString, DescriptorFactory>             _descriptorNames;          // XML descriptor name to descriptor factory
        std::multimap<UString, TID>                      _descriptorTablesIds;      // XML descriptor name to table id for table-specific descriptors
        std::map<EDID, DisplayDescriptorFunction>        _descriptorDisplays;       // Extended descriptor id to descriptor display
        std::map<uint16_t, DisplayCADescriptorFunction>  _casIdDescriptorDisplays;  // Key is CAS system id.
        UStringList                                      _xmlModelFiles;            // Additional XML model files for tables.
        UStringList                                      _namesFiles;               // Additional names files.

        // Register a new table id which matches standards and CAS ids.
        // Always return a non-null pointer (existing or newly created structure).
        TableDescription* registerTable(TID id, Standards standards, uint16_t minCAS, uint16_t maxCAS, std::initializer_list<PID> pids);

        // Common code to lookup a table function.
        template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type* = nullptr>
        FUNCTION getTableFunction(TID tid, Standards standards, PID pid, uint16_t cas, FUNCTION TableDescription::* member) const;

        // Common code to lookup a descriptor function.
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
#define TS_ID_TABLE_FACTORY(classname,id,std) _TS_TABLE_FACTORY(classname) TS_FACTORY_REGISTER((id), TS_UNIQUE_NAME(_Factory), (std))

//!
//! @hideinitializer
//! Registration of the table id of a subclass of ts::AbstractTable.
//! The table is defined by its standard to be on a given set of PID's.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_TABLE_PIDS_FACTORY(classname, id, std, ...) _TS_TABLE_FACTORY(classname) TS_FACTORY_REGISTER((id), TS_UNIQUE_NAME(_Factory), (std), {__VA_ARGS__})

//!
//! @hideinitializer
//! Registration of a range of table ids of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_ID_TABLE_RANGE_FACTORY(classname, minId, maxId, std) _TS_TABLE_FACTORY(classname) TS_FACTORY_REGISTER((minId), (maxId), TS_UNIQUE_NAME(_Factory), (std))

//!
//! @hideinitializer
//! Registration of the descriptor tag of a subclass of ts::AbstractDescriptor.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_ID_DESCRIPTOR_FACTORY(classname, id) _TS_DESCRIPTOR_FACTORY(classname) TS_FACTORY_REGISTER((id), TS_UNIQUE_NAME(_Factory))

//!
//! @hideinitializer
//! Registration of the XML name of a subclass of ts::AbstractTable.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_XML_TABLE_FACTORY(classname, xmlname) _TS_TABLE_FACTORY(classname) TS_FACTORY_REGISTER(xmlname, TS_UNIQUE_NAME(_Factory))

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
