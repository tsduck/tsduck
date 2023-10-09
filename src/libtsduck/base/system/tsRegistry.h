//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Windows Registry utilities.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsNullReport.h"

namespace ts {
    //!
    //! A class to encapsulate the access to the Windows registry.
    //! @ingroup windows
    //!
    //! Vocabulary :
    //!  - Key        : Node of the registry (kind of "directory").
    //!  - Value_Name : Name of a value in a key.
    //!  - Value      : Value of the Value_Name.
    //!
    //! This class is defined for all operating systems. So, it is possible
    //! to use it everywhere without complicated conditional compilation.
    //! However, all calls return an error on non-Windows systems.
    //!
    class TSDUCKDLL Registry
    {
    public:
        //!
        //! Handle to a registry key.
        //!
#if defined(DOXYGEN)
        typedef system-specific Handle;
#elif defined(TS_WINDOWS)
        typedef ::HKEY Handle;
#else
        typedef void* Handle;
#endif

        //!
        //! Get a value in a registry key as a string.
        //! @param [in] key Registry key.
        //! @param [in] value_name Name of the value in @a key.
        //! @param [in,out] report Where to report error.
        //! @return An empty string if non-existent or error.
        //!
        static UString GetValue(const UString& key, const UString& value_name = UString(), Report& report = NULLREP);

        //!
        //! Set the value of a registry key.
        //! @param [in] key Registry key.
        //! @param [in] value_name Name of the value in @a key.
        //! @param [in] value Value to set in @a value_name.
        //! @param [in] expandable If true, set the type to REG_EXPAND_SZ. Otherwise, set type to REG_SZ.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool SetValue(const UString& key,
                             const UString& value_name,
                             const UString& value,
                             bool expandable = false,
                             Report& report = NULLREP);

        //!
        //! Set value of a registry key.
        //! Set the data type as REG_DWORD.
        //! @param [in] key Registry key.
        //! @param [in] value_name Name of the value in @a key.
        //! @param [in] value Value to set in @a value_name. Set the data type as REG_DWORD.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool SetValue(const UString& key, const UString& value_name, uint32_t value, Report& report = NULLREP);

        //!
        //! Delete a value of a registry key.
        //! @param [in] key Registry key.
        //! @param [in] value_name Name of the value in @a key.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool DeleteValue(const UString& key, const UString& value_name, Report& report = NULLREP);

        //!
        //! Create a registry key.
        //! @param [in] key Registry key to create.
        //! @param [in] is_volatile If true, create a "volatile" registry key.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool CreateKey(const UString& key, bool is_volatile = false, Report& report = NULLREP);

        //!
        //! Delete a registry key.
        //! @param [in] key Registry key to delete.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool DeleteKey(const UString& key, Report& report = NULLREP);

        //!
        //! Get the root key of a registry path.
        //! @param [in] key Registry key to split.
        //! @param [out] root_key Handle to the root key.
        //! @param [out] subkey Subkey name.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool SplitKey(const UString& key, Handle& root_key, UString& subkey, Report& report = NULLREP);

        //!
        //! Get the root key of a registry path.
        //! @param [in] key Registry key to split.
        //! @param [out] root_key Handle to the root key.
        //! @param [out] midkey Middle key name (without root key and final component).
        //! @param [out] final_key Final component of the key.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool SplitKey(const UString& key, Handle& root_key, UString& midkey, UString& final_key, Report& report = NULLREP);

        //!
        //! Name of the registry key containining the system-defined environment variables.
        //!
        static const UString SystemEnvironmentKey;

        //!
        //! Name of the registry key containining the user-defined environment variables.
        //!
        static const UString UserEnvironmentKey;

        //!
        //! Notify all applications of a setting change.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool NotifySettingChange(Report& report = NULLREP);

        //!
        //! Notify all applications that the environment was modified.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool NotifyEnvironmentChange(Report& report = NULLREP);

    private:
#if defined(TS_WINDOWS)
        //!
        //! Open a registry key.
        //! @param [in] root_key Handle to the root key.
        //! @param [in] key Name of the key below root key.
        //! @param [in] sam Requested security access mask.
        //! @param [out] handle Handle to the key.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool OpenKey(Handle root, const UString& key, ::REGSAM sam, Handle& handle, Report& report);

        //!
        //! Notify all applications of a setting change.
        //! @param [in] param Notification parameter.
        //! @param [in] timeout_ms Timeout in milliseconds.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool NotifySettingChangeParam(const void* param, uint32_t timeout_ms, Report& report);
#endif
    };
}
