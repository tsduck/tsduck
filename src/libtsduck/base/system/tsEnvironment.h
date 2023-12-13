//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup system
//!  Accessing environment variables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Environment variable containing the command search path.
    //!
#if defined(DOXYGEN)
    constexpr const UChar* PATH_ENVIRONMENT_VARIABLE = platform - specific("PATH", "Path");  // for doc only
#elif defined(TS_WINDOWS)
    constexpr const UChar* PATH_ENVIRONMENT_VARIABLE = u"Path";
#elif defined(TS_UNIX)
    constexpr const UChar* PATH_ENVIRONMENT_VARIABLE = u"PATH";
#else
    #error "Unimplemented operating system"
#endif

    //!
    //! Name of the environment variable which contains a list of paths for plugins.
    //!
    constexpr const UChar* PLUGINS_PATH_ENVIRONMENT_VARIABLE = u"TSPLUGINS_PATH";

    //!
    //! Separator character in search paths.
    //!
#if defined(DOXYGEN)
    constexpr UChar SEARCH_PATH_SEPARATOR = platform-specific (':', ';'); // for doc only
#elif defined(TS_WINDOWS)
    constexpr UChar SEARCH_PATH_SEPARATOR = u';';
#elif defined(TS_UNIX)
    constexpr UChar SEARCH_PATH_SEPARATOR = u':';
#else
    #error "Unimplemented operating system"
#endif

    //!
    //! Check if an environment variable exists.
    //! @param [in] varname Environment variable name.
    //! @return True if the specified environment variable exists, false otherwise.
    //!
    TSDUCKDLL bool EnvironmentExists(const UString& varname);

    //!
    //! Get the value of an environment variable.
    //! @param [in] varname Environment variable name.
    //! @param [in] defvalue Default value if the specified environment variable does not exist.
    //! @return The value of the specified environment variable it it exists, @a defvalue otherwise.
    //!
    TSDUCKDLL UString GetEnvironment(const UString& varname, const UString& defvalue = UString());

    //!
    //! Get the value of an environment variable containing a search path.
    //!
    //! The search path is analyzed and split into individual directory names.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [out] container A container of @c UString receiving the directory names.
    //! @param [in] name Environment variable name.
    //! @param [in] def Default value if the specified environment variable does not exist.
    //!
    template <class CONTAINER>
    inline void GetEnvironmentPath(CONTAINER& container, const UString& name, const UString& def = UString())
    {
        GetEnvironment(name, def).split(container, SEARCH_PATH_SEPARATOR, true, true);
    }

    //!
    //! Get the value of an environment variable containing a search path.
    //!
    //! The search path is analyzed and split into individual directory names.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [in,out] container A container of @c UString receiving the directory names.
    //! The directory names are appended to the container without erasing previous content.
    //! @param [in] name Environment variable name.
    //! @param [in] def Default value if the specified environment variable does not exist.
    //!
    template <class CONTAINER>
    inline void GetEnvironmentPathAppend(CONTAINER& container, const UString& name, const UString& def = UString())
    {
        GetEnvironment(name, def).splitAppend(container, SEARCH_PATH_SEPARATOR, true, true);
    }

    //!
    //! Set the value of an environment variable.
    //!
    //! If the variable previously existed, its value is overwritten.
    //! If it did not exist, it is created.
    //!
    //! @param [in] name Environment variable name.
    //! @param [in] value Environment variable value.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool SetEnvironment(const UString& name, const UString& value);

    //!
    //! Set the value of an environment variable containing a search path.
    //!
    //! If the variable previously existed, its value is overwritten.
    //! If it did not exist, it is created.
    //!
    //! @tparam CONTAINER A container class of @c UString as defined by the C++ Standard Template Library (STL).
    //! @param [in] name Environment variable name.
    //! @param [in] container A container of @c UString containing directory names.
    //!
    template <class CONTAINER>
    inline void SetEnvironmentPath(const UString& name, const CONTAINER& container)
    {
        SetEnvironment(name, UString::Join(container, UString(1, SEARCH_PATH_SEPARATOR)));
    }

    //!
    //! Delete an environment variable.
    //!
    //! If the variable did not exist, do nothing, do not generate an error.
    //!
    //! @param [in] name Environment variable name.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool DeleteEnvironment(const UString& name);

    //!
    //! Expand environment variables inside a file path (or any string).
    //!
    //! Environment variable references '$name' or '${name}' are replaced
    //! by the corresponding values from the environment.
    //! In the first form, 'name' is the longest combination of letters, digits and underscore.
    //! A combination \\$ is interpreted as a literal $, not an environment variable reference.
    //!
    //! @param [in] path A path string containing references to environment variables.
    //! @return The expanded string.
    //!
    TSDUCKDLL UString ExpandEnvironment(const UString& path);

    //!
    //! Define a container type holding all environment variables.
    //!
    //! For each element in the container, the @e key is the name of an
    //! environment variable and the @e value is the corresponding value
    //! of this environment variable.
    //!
    typedef std::map<UString, UString> Environment;

    //!
    //! Get the content of the entire environment (all environment variables).
    //!
    //! @param [out] env An associative container which receives the content
    //! of the environment. Each @e key is the name of an environment variable
    //! and the corresponding @e value is the value of this environment variable.
    //!
    TSDUCKDLL void GetEnvironment(Environment& env);

    //!
    //! Load a text file containing environment variables.
    //! Each line shall be in the form "name = value".
    //! Empty line and line starting with '#' are ignored.
    //! Spaces are trimmed.
    //!
    //! @param [out] env An associative container which receives the content of the environment.
    //! Each @e key is the name of an environment variable and the corresponding @e value is
    //! the value of this environment variable.
    //! @param [in] fileName Name of the file to load.
    //! @return True on success, false on error.
    //!
    TSDUCKDLL bool LoadEnvironment(Environment& env, const UString& fileName);
}
