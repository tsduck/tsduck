//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A repository for known PSI/SI tables and descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsEDID.h"
#include "tsStandards.h"
#include "tsTablesPtr.h"
#include "tsSingleton.h"
#include "tsVersionInfo.h"

namespace ts {

    class DuckContext;
    class TablesDisplay;
    class DVBCharTable;

    //!
    //! A repository for known PSI/SI tables and descriptors.
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
    class TSDUCKDLL PSIRepository
    {
        TS_DECLARE_SINGLETON(PSIRepository);
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
        //! A class to register fully implemented tables.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL RegisterTable
        {
            TS_NOBUILD_NOCOPY(RegisterTable);
        public:
            //!
            //! Register a fully implemented table.
            //! @param [in] factory Function which creates a table of this type.
            //! @param [in] tids List of table ids for this type. Usually there is only one (notable exception: EIT, SDT, NIT).
            //! @param [in] standards List of standards which define this table.
            //! @param [in] xmlName XML node name for this table type.
            //! @param [in] displayFunction Display function for the corresponding sections. Can be null.
            //! @param [in] logFunction Log function for the corresponding sections. Can be null.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only. Same as @a minCAS when set as CASID_NULL.
            //! @see TS_REGISTER_TABLE
            //!
            RegisterTable(TableFactory factory,
                          const std::vector<TID>& tids,
                          Standards standards,
                          const UString& xmlName,
                          DisplaySectionFunction displayFunction = nullptr,
                          LogSectionFunction logFunction = nullptr,
                          std::initializer_list<PID> pids = {},
                          uint16_t minCAS = CASID_NULL,
                          uint16_t maxCAS = CASID_NULL);

            //!
            //! Register a known table with display functions but no full C++ class.
            //! @param [in] tids List of table ids for this type. Usually there is only one (notable exception: EIT, SDT, NIT).
            //! @param [in] standards List of standards which define this table.
            //! @param [in] displayFunction Display function for the corresponding sections. Can be null.
            //! @param [in] logFunction Log function for the corresponding sections. Can be null.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only. Same as @a minCAS when set as CASID_NULL.
            //! @see TS_REGISTER_SECTION
            //!
            RegisterTable(const std::vector<TID>& tids,
                          Standards standards,
                          DisplaySectionFunction displayFunction = nullptr,
                          LogSectionFunction logFunction = nullptr,
                          std::initializer_list<PID> pids = {},
                          uint16_t minCAS = CASID_NULL,
                          uint16_t maxCAS = CASID_NULL);
        };

        //!
        //! A class to register fully implemented descriptors.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL RegisterDescriptor
        {
            TS_NOBUILD_NOCOPY(RegisterDescriptor);
        public:
            //!
            //! Register a descriptor factory for a given descriptor tag.
            //! @param [in] factory Function which creates a descriptor of this type.
            //! @param [in] edid Exended descriptor id.
            //! @param [in] xmlName XML node name for this descriptor type.
            //! @param [in] displayFunction Display function for the corresponding descriptors. Can be null.
            //! @param [in] xmlNameLegacy Legacy XML node name for this descriptor type (optional).
            //! @see TS_REGISTER_DESCRIPTOR
            //!
            RegisterDescriptor(DescriptorFactory factory,
                               const EDID& edid,
                               const UString& xmlName,
                               DisplayDescriptorFunction displayFunction = nullptr,
                               const UString& xmlNameLegacy = UString());

            //!
            //! Registers a CA_descriptor display function for a given range of CA_system_id.
            //! @param [in] displayFunction Display function for the corresponding descriptors.
            //! @param [in] minCAS First CA_system_id if the display function applies to one CAS only.
            //! @param [in] maxCAS Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @see TS_REGISTER_CA_DESCRIPTOR
            //!
            RegisterDescriptor(DisplayCADescriptorFunction displayFunction, uint16_t minCAS, uint16_t maxCAS = CASID_NULL);

        private:
            void registerXML(DescriptorFactory factory, const EDID& edid, const UString& xmlName, const UString& xmlNameLegacy);
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
            //! @see TS_REGISTER_XML_FILE
            //!
            RegisterXML(const UString& filename);
        };

    private:
        // Description of a table id. Several descriptions can be used for the same table id,
        // for instance for distinct DTV standards or disctinct CA systems.
        // We use a fixed-size array for 'pids' instead of a PIDSet for storage efficiency.
        class TableDescription
        {
        public:
            Standards              standards = Standards::NONE;  // Standards for this table id.
            uint16_t               minCAS = CASID_NULL;          // Minimum CAS id for this table id (CASID_NULL if none).
            uint16_t               maxCAS = CASID_NULL;          // Maximum CAS id for this table id (CASID_NULL if none).
            TableFactory           factory = nullptr;            // Function to build an instance of the table.
            DisplaySectionFunction display = nullptr;            // Function to display a section.
            LogSectionFunction     log = nullptr;                // Function to log a section.
            std::array<PID,8>      pids {};                      // Standard PID's for the standard, stop at first PID_NULL.

            // Constructor.
            TableDescription();

            // Add PIDs in the list.
            void addPIDs(std::initializer_list<PID> morePIDs);

            // Check if a PID is present.
            bool hasPID(PID pid) const;
        };

        // Description of a descriptor extended id.
        // Only one description can be used per extended descriptor id,
        class DescriptorDescription
        {
        public:
            DescriptorFactory         factory;  // Function to build an instance of the descriptor.
            DisplayDescriptorFunction display;  // Function to display a descriptor.

            // Constructor.
            DescriptorDescription(DescriptorFactory fact = nullptr, DisplayDescriptorFunction disp = nullptr);
        };

        // PSIRepository instance private members.
        std::multimap<TID, TableDescription>            _tables {};                   // Description of all table ids, potential multiple entries per table idx
        std::map<EDID, DescriptorDescription>           _descriptors {};              // Description of all descriptors, by extended id.
        std::map<UString, TableFactory>                 _tableNames {};               // XML table name to table factory
        std::map<UString, DescriptorFactory>            _descriptorNames {};          // XML descriptor name to descriptor factory
        std::multimap<UString, TID>                     _descriptorTablesIds {};      // XML descriptor name to table id for table-specific descriptors
        std::map<uint16_t, DisplayCADescriptorFunction> _casIdDescriptorDisplays {};  // CA_system_id to display function for CA_descriptor.
        UStringList                                     _xmlModelFiles {};            // Additional XML model files for tables.

        // Common code to lookup a table function.
        template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type* = nullptr>
        FUNCTION getTableFunction(TID tid, Standards standards, PID pid, uint16_t cas, FUNCTION TableDescription::* member) const;

        // Common code to lookup a descriptor function.
        template <typename FUNCTION, typename std::enable_if<std::is_pointer<FUNCTION>::value>::type* = nullptr>
        FUNCTION getDescriptorFunction(const EDID& edid, TID tid, FUNCTION DescriptorDescription::* member) const;
    };
}

//
// Implementation note: Take care before modifying the following macros.
// Especially, the *_FACTORY macros need to be defined on one single line
// each because of the use of __LINE__ to create unique identifiers.
//

//! @cond nodoxygen
#define _TS_FACTORY_NAME                  TS_UNIQUE_NAME(_Factory)
#define _TS_REGISTRAR_NAME                TS_UNIQUE_NAME(_Registrar)
#define _TS_FACTORY(rettype,classname)    namespace { rettype _TS_FACTORY_NAME() {return new classname;} }
#define _TS_TABLE_FACTORY(classname)      _TS_FACTORY(ts::AbstractTablePtr, classname)
#define _TS_DESCRIPTOR_FACTORY(classname) _TS_FACTORY(ts::AbstractDescriptorPtr, classname)
//! @endcond

//!
//! @hideinitializer
//! Registration of a fully implemented table inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_REGISTER_TABLE(classname, ...) \
    TS_LIBCHECK(); \
    _TS_TABLE_FACTORY(classname) static ts::PSIRepository::RegisterTable _TS_REGISTRAR_NAME(_TS_FACTORY_NAME, __VA_ARGS__)

//!
//! @hideinitializer
//! Registration of a known table with display functions but no full C++ class.
//! This macro is typically used in the .cpp file of a CAS-specific module or TSDuck extension.
//!
#define TS_REGISTER_SECTION(...) \
    TS_LIBCHECK(); \
    static ts::PSIRepository::RegisterTable _TS_REGISTRAR_NAME(__VA_ARGS__)

//!
//! @hideinitializer
//! Registration of a fully implemented descriptor inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_REGISTER_DESCRIPTOR(classname, ...) \
    TS_LIBCHECK(); \
    _TS_DESCRIPTOR_FACTORY(classname) static ts::PSIRepository::RegisterDescriptor _TS_REGISTRAR_NAME(_TS_FACTORY_NAME, __VA_ARGS__)

//!
//! @hideinitializer
//! Registration of a display function for a CA_descriptor inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a CAS-specific module or TSDuck extension.
//!
#define TS_REGISTER_CA_DESCRIPTOR(func, ...) \
    TS_LIBCHECK(); \
    static ts::PSIRepository::RegisterDescriptor _TS_REGISTRAR_NAME(func, __VA_ARGS__)

//!
//! @hideinitializer
//! Registration of an extension XML model file inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a TSDuck extension.
//!
#define TS_REGISTER_XML_FILE(filename) \
    TS_LIBCHECK(); \
    static ts::PSIRepository::RegisterXML _TS_REGISTRAR_NAME(filename)
