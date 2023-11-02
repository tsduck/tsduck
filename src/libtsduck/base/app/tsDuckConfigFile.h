//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A singleton which contains the TSDuck configuration file.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsConfigFile.h"
#include "tsSingleton.h"

namespace ts {
    //!
    //! A singleton which contains the TSDuck configuration file.
    //! @ingroup app
    //!
    class TSDUCKDLL DuckConfigFile : public ConfigFile
    {
        // This class is a singleton. Use static Instance() method.
        TS_DECLARE_SINGLETON(DuckConfigFile);

    public:
        //!
        //! Get the value of an entry.
        //! A section with the name of the executable is searched first.
        //! Then, the global section is used.
        //! @param [in] entry Entry name.
        //! @param [in] defvalue Default value.
        //! @return The value in the entry or @a defvalue if @a entry does not exist.
        //!
        UString value(const UString& entry, const UString& defvalue = UString()) const;

        //!
        //! Get all values of an entry.
        //! A section with the name of the executable is searched first.
        //! Then, the global section is used.
        //! @param [in] entry Entry name.
        //! @param [out] values Vector of values.
        //!
        void getValues(const UString& entry, UStringVector& values) const;

    private:
        const UString        _appName;
        const ConfigSection& _appSection;
        const ConfigSection& _mainSection;
    };
}
