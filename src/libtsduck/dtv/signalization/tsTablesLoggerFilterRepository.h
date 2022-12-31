//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//!  The repository of section filters for TablesLogger.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTablesLoggerFilterInterface.h"
#include "tsSingletonManager.h"

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
        std::vector<Register::FilterFactory> _factories;
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
