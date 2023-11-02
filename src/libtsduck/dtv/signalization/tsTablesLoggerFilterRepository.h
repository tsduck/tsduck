//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  The repository of section filters for TablesLogger.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTablesLoggerFilterInterface.h"
#include "tsSingleton.h"

namespace ts {
    //!
    //! The repository of section filters for TablesLogger.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TablesLoggerFilterRepository
    {
        TS_DECLARE_SINGLETON(TablesLoggerFilterRepository);
    public:
        //!
        //! Create an instance of all registered section filters.
        //! @param [out] filters Returned vectors of safe pointers to instances of section filters.
        //!
        void createFilters(TablesLoggerFilterVector& filters) const;

        //!
        //! A class to register factories of section filters.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSDUCKDLL Register
        {
            TS_NOBUILD_NOCOPY(Register);
        public:
            //!
            //! Profile of a function which creates a TablesLogger section filter.
            //! @return A pointer to an instance of a concrete subclass of TablesLoggerFilterInterface.
            //!
            typedef TablesLoggerFilterInterface* (*FilterFactory)();
            //!
            //! The constructor registers a section filter factory.
            //! @param [in] factory Function which creates a section filter.
            //!
            Register(FilterFactory factory);
        };

    private:
        std::vector<Register::FilterFactory> _factories {};
    };
}

//!
//! @hideinitializer
//! Registration inside the ts::TablesLoggerFilterRepository singleton.
//! This macro is typically used in the .cpp file of a table or descriptor.
//!
// Implementation note: Take care before modifying the following macro.
// It must be defined on one single line each because of the use of __LINE__ to create unique identifiers.
//
#define TS_REGISTER_SECTION_FILTER(classname) \
    namespace { ts::TablesLoggerFilterInterface* TS_UNIQUE_NAME(_Factory)() {return new classname;} } static ts::TablesLoggerFilterRepository::Register TS_UNIQUE_NAME(_Registrar)(TS_UNIQUE_NAME(_Factory))
