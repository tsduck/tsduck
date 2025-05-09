//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A repository for known PSI/SI tables and descriptors.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEDID.h"
#include "tsNames.h"
#include "tsTablesPtr.h"
#include "tsSectionContext.h"
#include "tsDescriptorContext.h"
#include "tsLibTSDuckVersion.h"

namespace ts {

    class DuckContext;
    class TablesDisplay;

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
    //! Mixed ISDB-DVB compatibility. ISDB is based on a subset of DVB and adds other tables and
    //! descriptors. The DVB subset is compatible with ISDB. When another DID or TID is defined
    //! with two distinct semantics, one for DVB and one for ISDB, if ISDB is part of the current
    //! standards we use the ISDB semantics, otherwise we use the DVB semantics.
    //!
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL PSIRepository : private Names::Visitor
    {
        TS_SINGLETON(PSIRepository);
    public:
        //!
        //! Profile of a function which creates a table.
        //! @return A safe pointer to an instance of a concrete subclass of AbstractTable.
        //!
        using TableFactory = AbstractTablePtr (*)();

        //!
        //! Profile of a function which creates a descriptor.
        //! @return A safe pointer to an instance of a concrete subclass of AbstractDescriptor.
        //!
        using DescriptorFactory = AbstractDescriptorPtr (*)();

        //!
        //! Get a value of a "null" type index.
        //! The same value is always returned and can be used as a placeholder for an unused value.
        //! @return A value of a "null" type index.
        //!
        static std::type_index NullIndex() { return std::type_index(typeid(std::nullptr_t)); }

        //!
        //! Base description of a signalization class, common to tables and descriptors.
        //!
        class TSDUCKDLL SignalizationClass
        {
            TS_INTERFACE(SignalizationClass);
        public:
            std::type_index index = NullIndex();  //!< RTTI type index for the C++ class.
            UString         display_name {};      //!< Displayable name for that table or descriptor.
            UString         xml_name {};          //!< XML name for that table or descriptor.

            //!
            //! Get the applicable standards for the table or descriptor.
            //! @return The set of applicable standards for the table or descriptor.
            //!
            virtual Standards getStandards() const = 0;
        };

        //!
        //! Description of a table class.
        //!
        class TSDUCKDLL TableClass : public SignalizationClass
        {
        public:
            Standards              standards = Standards::NONE;  //!< Standards for this table id.
            CASID                  min_cas = CASID_NULL;         //!< Minimum CAS id for this table id (CASID_NULL if none).
            CASID                  max_cas = CASID_NULL;         //!< Maximum CAS id for this table id (CASID_NULL if none).
            TableFactory           factory = nullptr;            //!< Function to build an instance of the table.
            DisplaySectionFunction display = nullptr;            //!< Function to display a section.
            LogSectionFunction     log = nullptr;                //!< Function to log a section.
            std::set<PID>          pids {};                      //!< Standard PID's for the table.

            //! @cond nodoxygen
            virtual Standards getStandards() const override;
            virtual ~TableClass() override;
            //! @endcond
        };

        //!
        //! Description of a descriptor class.
        //!
        class TSDUCKDLL DescriptorClass : public SignalizationClass
        {
        public:
            EDID                      edid {};             //!< Extended descriptor id.
            DescriptorFactory         factory = nullptr;   //!< Function to build an instance of the descriptor.
            DisplayDescriptorFunction display = nullptr;   //!< Function to display a descriptor.
            UString                   legacy_xml_name {};  //!< Optional legacy XML name for that descriptor.

            //! @cond nodoxygen
            virtual Standards getStandards() const override;
            virtual ~DescriptorClass() override;
            //! @endcond
        };

        //!
        //! Get the description of a table class for a given table id and context.
        //! @param [in] tid Table id.
        //! @param [in] context Optional object to lookup the context of the table.
        //! This may help disambiguate tables with distinct standards but identical table_id.
        //! @return A constant reference to the description of the class. If the class is not
        //! found, the returned description is empty (same as initial state of a TableClass).
        //!
        const TableClass& getTable(TID tid, const SectionContext& context = SectionContext()) const;

        //!
        //! Get the description of a table class for a given XML node name.
        //! @param [in] xml_name Name of XML node.
        //! @return A constant reference to the description of the class. If the class is not
        //! found, the returned description is empty (same as initial state of a TableClass).
        //!
        const TableClass& getTable(const UString& xml_name) const;

        //!
        //! Get the list of standards which are defined for a given table id.
        //! @param [in] id Table id.
        //! @param [in] pid PID on which the section is found.
        //! @param [in] current_standards Current standards in the stream so far. Any
        //! potential table which belongs to an incompatible standard is ignored.
        //! @return Corresponding list of standards. If multiple definitions exist for this
        //! table id, return the common subset of all definitions. This means that only
        //! standards which are used in all cases are returned.
        //!
        Standards getTableStandards(TID id, PID pid = PID_NULL, Standards current_standards = Standards::NONE) const;

        //!
        //! Get the description of a descriptor class for a given EDID.
        //! @param [in] edid Extended descriptor id.
        //! @return A constant reference to the description of the class. If the class is not
        //! found, the returned description is empty (same as initial state of a DescriptorClass).
        //!
        const DescriptorClass& getDescriptor(EDID edid) const;

        //!
        //! Get the description of a descriptor class for a given descriptor tag and its context.
        //! @param [in] xdid Extension descriptor id. This value is extracted from the descriptor itself.
        //! @param [in,out] context Object to lookup the context of the descriptor.
        //! @return A constant reference to the description of the class. If the class is not
        //! found, the returned description is empty (same as initial state of a DescriptorClass).
        //!
        const DescriptorClass& getDescriptor(XDID xdid, DescriptorContext& context) const;

        //!
        //! Get the description of a descriptor class for a given descriptor closs RTTI index.
        //! Useful for a descriptor class to get its own description.
        //! @param [in] index RTTI index for the descriptor class.
        //! @param [in] tid Optional TID. If the descriptor class is table-specific for
        //! several tables, return the EDID for that table. If @a tid is TID_NULL and there
        //! are several specific tables for that descriptor, the first one is returned.
        //! @param [in] standards Optional list of standards for the table.
        //! @return A constant reference to the description of the class. If the class is not
        //! found, the returned description is empty (same as initial state of a DescriptorClass).
        //!
        const DescriptorClass& getDescriptor(std::type_index index, TID tid = TID_NULL, Standards standards = Standards::NONE) const;

        //!
        //! Get the description of a descriptor class for a given XML node name.
        //! @param [in] xml_name Name of XML node.
        //! @return A constant reference to the description of the class. If the class is not
        //! found, the returned description is empty (same as initial state of a DescriptorClass).
        //!
        const DescriptorClass& getDescriptor(const UString& xml_name) const;

        //!
        //! Check if a descriptor is allowed in a table.
        //! @param [in] xml_name Name of the XML node for the descriptor.
        //! @param [in] tid Table id of the table to check.
        //! @return True if the descriptor is allowed, false otherwise.
        //! Non-table-specific descriptors are allowed everywhere.
        //! Table-specific descriptors are allowed only in a set of specific tables.
        //!
        bool isDescriptorAllowed(const UString& xml_name, TID tid) const;

        //!
        //! Get the list of tables where a descriptor is allowed, as a descriptive string.
        //! @param [in] duck TSDuck execution context to interpret table names.
        //! @param [in] nodeName Name of the XML node for the descriptor.
        //! @return Human-readable list of tables where the descriptor is allowed.
        //! Empty string for non-table-specific descriptors.
        //!
        UString descriptorTables(const DuckContext& duck, const UString& nodeName) const;

        //!
        //! Get the display function of the CA_descriptor for a given CA_system_id.
        //! @param [in] cas CA_system_id.
        //! @return Corresponding display function or zero if there is none.
        //!
        DisplayCADescriptorFunction getCADescriptorDisplay(CASID cas) const;

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
            //! @param [in] factory Function which creates a table object of this type.
            //! @param [in] index Type index of the table object class.
            //! @param [in] tids List of table ids for this type. Usually there is only one (notable exception: EIT, SDT, NIT).
            //! @param [in] standards List of standards which define this table.
            //! @param [in] xml_name XML node name for this table type.
            //! @param [in] display Display function for the corresponding sections. Can be null.
            //! @param [in] log Log function for the corresponding sections. Can be null.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @param [in] min_cas First CA_system_id if the display function applies to one CAS only.
            //! @param [in] max_cas Last CA_system_id if the display function applies to one CAS only. Same as @a minCAS when set as CASID_NULL.
            //! @see TS_REGISTER_TABLE
            //!
            RegisterTable(TableFactory factory,
                          std::type_index index,
                          const std::vector<TID>& tids,
                          Standards standards,
                          const UString& xml_name,
                          DisplaySectionFunction display = nullptr,
                          LogSectionFunction log = nullptr,
                          std::initializer_list<PID> pids = {},
                          CASID min_cas = CASID_NULL,
                          CASID max_cas = CASID_NULL);

            //!
            //! Register a known table with display functions but no full C++ class.
            //! @param [in] tids List of table ids for this type. Usually there is only one (notable exception: EIT, SDT, NIT).
            //! @param [in] standards List of standards which define this table.
            //! @param [in] display Display function for the corresponding sections. Can be null.
            //! @param [in] log Log function for the corresponding sections. Can be null.
            //! @param [in] pids List of PID's which are defined by the standards for this table.
            //! @param [in] min_cas First CA_system_id if the display function applies to one CAS only.
            //! @param [in] max_cas Last CA_system_id if the display function applies to one CAS only. Same as @a minCAS when set as CASID_NULL.
            //! @see TS_REGISTER_SECTION
            //!
            RegisterTable(const std::vector<TID>& tids,
                          Standards standards,
                          DisplaySectionFunction display = nullptr,
                          LogSectionFunction log = nullptr,
                          std::initializer_list<PID> pids = {},
                          CASID min_cas = CASID_NULL,
                          CASID max_cas = CASID_NULL);
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
            //! @param [in] factory Function which creates a descriptor object of this type.
            //! @param [in] index Type index of the descriptor object class.
            //! @param [in] edid Exended descriptor id.
            //! @param [in] xml_name XML node name for this descriptor type.
            //! @param [in] display Display function for the corresponding descriptors. Can be null.
            //! @param [in] legacy_xml_name Legacy XML node name for this descriptor type (optional).
            //! @see TS_REGISTER_DESCRIPTOR
            //!
            RegisterDescriptor(DescriptorFactory factory,
                               std::type_index index,
                               const EDID& edid,
                               const UString& xml_name,
                               DisplayDescriptorFunction display = nullptr,
                               const UString& legacy_xml_name = UString());

            //!
            //! Registers a CA_descriptor display function for a given range of CA_system_id.
            //! @param [in] display Display function for the corresponding descriptors.
            //! @param [in] min_cas First CA_system_id if the display function applies to one CAS only.
            //! @param [in] max_cas Last CA_system_id if the display function applies to one CAS only.
            //! Same @a minCAS when set as CASID_NULL.
            //! @see TS_REGISTER_CA_DESCRIPTOR
            //!
            RegisterDescriptor(DisplayCADescriptorFunction display, CASID min_cas, CASID max_cas = CASID_NULL);
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
            //! @param [in] file_name Name of the XML model file. This should be a simple file name,
            //! without directory. This file will be searched in the same directory as the executable,
            //! then in all directories from $TSPLUGINS_PATH, then from $LD_LIBRARY_PATH (Linux only),
            //! then from $PATH.
            //! @see TS_REGISTER_XML_FILE
            //!
            RegisterXML(const UString& file_name);
        };

        //!
        //! List all supported tables.
        //! @param [in,out] out Output stream.
        //!
        void listTables(std::ostream& out) const;

        //!
        //! List all supported descriptors.
        //! @param [in,out] out Output stream.
        //!
        void listDescriptors(std::ostream& out) const;

        //!
        //! Dump the internal state of the PSI repository (for debug only).
        //! @param [in,out] out Output stream.
        //!
        void dumpInternalState(std::ostream& out) const;

    private:
        using TableClassPtr = std::shared_ptr<TableClass>;
        using DescriptorClassPtr = std::shared_ptr<DescriptorClass>;

        // Several table classes can be used for the same table id, for instance for distinct DTV standards or
        // distinct CA systems. There is only one class per XML name.
        std::multimap<TID, TableClassPtr> _tables_by_tid {};
        std::map<UString, TableClassPtr>  _tables_by_xml_name {};

        // Several descriptor classes can be used for the same descriptor id (private, extended, table-specific descriptors).
        std::multimap<XDID, DescriptorClassPtr>            _descriptors_by_xdid {};        // Description of all descriptors, by XDID (multiple entries per XDID).
        std::map<UString, DescriptorClassPtr>              _descriptors_by_xml_name {};    // Description of all descriptors, by XML name (including legacy names).
        std::multimap<std::type_index, DescriptorClassPtr> _descriptors_by_type_index {};  // Description of all descriptors, by RTTI type index (multiple entries if multiple EDID).
        std::multimap<UString, TID>                        _descriptor_tids {};            // XML descriptor name to table id for table-specific descriptors

        // Display functions for CA_descriptor by CA_system_id.
        std::map<uint16_t, DisplayCADescriptorFunction> _casid_descriptor_displays {};

        // Additional XML model files for tables and descriptors.
        UStringList _xml_extension_files {};

        // Implementation of Names::Visitor.
        virtual bool handleNameValue(const Names& section, Names::uint_t value, const UString& name) override;

        // Display utilities.
        static UString NameToString(const UString& prefix, const UString& name, const UString& suffix);
        static UString TypeIndexToString(std::type_index index);
        static UString StandardsToString(Standards std);
        static UString PIDsToString(const std::set<PID>& pids);
        static UString CASToString(CASID min, CASID max);
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
#define _TS_FACTORY(rettype,classname)    namespace { rettype _TS_FACTORY_NAME() { return rettype(new classname); } }
#define _TS_TABLE_FACTORY(classname)      _TS_FACTORY(ts::AbstractTablePtr, classname)
#define _TS_DESCRIPTOR_FACTORY(classname) _TS_FACTORY(ts::AbstractDescriptorPtr, classname)
//! @endcond

//!
//! @hideinitializer
//! Registration of a fully implemented table inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a table.
//!
#define TS_REGISTER_TABLE(classname, ...) \
    TS_LIBTSDUCK_CHECK(); \
    _TS_TABLE_FACTORY(classname) static ts::PSIRepository::RegisterTable _TS_REGISTRAR_NAME(_TS_FACTORY_NAME, std::type_index(typeid(classname)), __VA_ARGS__)

//!
//! @hideinitializer
//! Registration of a known table with display functions but no full C++ class.
//! This macro is typically used in the .cpp file of a CAS-specific module or TSDuck extension.
//!
#define TS_REGISTER_SECTION(...) \
    TS_LIBTSDUCK_CHECK(); \
    static ts::PSIRepository::RegisterTable _TS_REGISTRAR_NAME(__VA_ARGS__)

//!
//! @hideinitializer
//! Registration of a fully implemented descriptor inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a descriptor.
//!
#define TS_REGISTER_DESCRIPTOR(classname, ...) \
    TS_LIBTSDUCK_CHECK(); \
    _TS_DESCRIPTOR_FACTORY(classname) static ts::PSIRepository::RegisterDescriptor _TS_REGISTRAR_NAME(_TS_FACTORY_NAME, std::type_index(typeid(classname)), __VA_ARGS__)

//!
//! @hideinitializer
//! Registration of a display function for a CA_descriptor inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a CAS-specific module or TSDuck extension.
//!
#define TS_REGISTER_CA_DESCRIPTOR(func, ...) \
    TS_LIBTSDUCK_CHECK(); \
    static ts::PSIRepository::RegisterDescriptor _TS_REGISTRAR_NAME(func, __VA_ARGS__)

//!
//! @hideinitializer
//! Registration of an extension XML model file inside the ts::PSIRepository singleton.
//! This macro is typically used in the .cpp file of a TSDuck extension.
//!
#define TS_REGISTER_XML_FILE(filename) \
    TS_LIBTSDUCK_CHECK(); \
    static ts::PSIRepository::RegisterXML _TS_REGISTRAR_NAME(filename)
