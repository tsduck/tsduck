//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of a Uniform Resource Locator (URL).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Representation of a Uniform Resource Locator (URL).
    //! @ingroup net
    //!
    class TSDUCKDLL URL
    {
    public:
        //!
        //! Default constructor.
        //!
        URL() = default;

        //!
        //! Constructor from a string.
        //! @param [in] path A file path or URL as a string.
        //! If this is a file specification, build a "file:" URL with the absolute path.
        //!
        URL(const UString& path) { setURL(path); }

        //!
        //! Constructor from a string and a base string.
        //! @param [in] path A file path or URL as a string.
        //! @param [in] base The base URL or directory to use if @a path is a relative file path.
        //! By default, when @a base is empty, the current working directory is used and a "file:"
        //! URL is built.
        //!
        URL(const UString& path, const UString& base) { setURL(path, base); }

        //!
        //! Constructor from a string and a base URL.
        //! @param [in] path A file path or URL as a string.
        //! @param [in] base The base URL to use if @a path is a relative path.
        //!
        URL(const UString& path, const URL& base) { setURL(path, base); }

        //!
        //! Set URL from a string.
        //! @param [in] path A file path or URL as a string.
        //! If this is a file specification, build a "file:" URL with the absolute path.
        //!
        void setURL(const UString& path);

        //!
        //! Set URL from a string and a base string.
        //! @param [in] path A file path or URL as a string.
        //! @param [in] base The base URL or directory to use if @a path is a relative file path.
        //! By default, when @a base is empty, the current working directory is used and a "file:"
        //! URL is built.
        //!
        void setURL(const UString& path, const UString& base);

        //!
        //! Set URL from a string and a base URL.
        //! @param [in] path A file path or URL as a string.
        //! @param [in] base The base URL to use if @a path is a relative path.
        //!
        void setURL(const UString& path, const URL& base);

        //!
        //! Convert to a string object.
        //! @param [in] useWinInet This boolean is used on Windows only. When true, a file
        //! URL is built as 'file://C:/dir/file' (with 2 slashes). When false, the URL is
        //! 'file:///C:/dir/file' (with 3 slashes). The latter form is the documented one
        //! from Microsoft and should be considered as the "correct" one. However, the
        //! Microsoft WinInet library (which is used by the WebRequest class) requires
        //! the incorrect form with 2 slashes. So, if the resulting URL is to be used
        //! by WebRequest, set @a useWinInet to true (the default) but if the URL needs
        //! to be published somewhere, use false.
        //! @return The URL as a string.
        //!
        UString toString(bool useWinInet = true) const;

        //!
        //! Extract a relative URL of this object, from a base URL.
        //! @param [in] base The base URL to use. If this object and @a base share the
        //! same scheme and host specification.
        //! @param [in] useWinInet This boolean is used on Windows only.
        //! See toString() for details.
        //! @return The relative URL as a string.
        //!
        UString toRelative(const URL& base, bool useWinInet = true) const;

        //!
        //! Extract a relative URL of this object, from a base URL.
        //! @param [in] base The base URL to use. If this object and @a base share the
        //! same scheme and host specification.
        //! @param [in] useWinInet This boolean is used on Windows only.
        //! See toString() for details.
        //! @return The relative URL as a string.
        //!
        UString toRelative(const UString& base, bool useWinInet = true) const;

        //!
        //! Check if two URL's use the same server (scheme, host, user, etc.)
        //! @param [in] other Another URL to compare.
        //! @return True if the two URL' as a string's use the same server.
        //!
        bool sameServer(const URL& other) const;

        //!
        //! Check if the URL is valid (was built from a valid URL string).
        //! @return True if the URL is valid.
        //!
        bool isValid() const { return !_scheme.empty(); }

        //!
        //! Clear the content of the URL (becomes invalid).
        //!
        void clear();

        //!
        //! Define a URL property accessors.
        //! This macro is for class internal use only.
        //! @param type C++ type for the property.
        //! @param suffix Accessor methods suffix.
        //! @param field Internal class private field.
        //! @param fullname Explanatory description of the property.
        //! @hideinitializer
        //!
#define URL_PROPERTY(type,suffix,field,fullname)               \
        /** Set the fullname.                               */ \
        /** @param [in] value The fullname.                 */ \
        void set##suffix(const type& value) { field = value; } \
        /** Get the fullname.                               */ \
        /** @return The fullname.                           */ \
        type get##suffix() const { return field; }

        URL_PROPERTY(UString, Scheme, _scheme, scheme name without trailing colon)
        URL_PROPERTY(UString, UserName, _username, optiona user name part)
        URL_PROPERTY(UString, Password, _password, optional password)
        URL_PROPERTY(UString, Host, _host, host name)
        URL_PROPERTY(uint16_t, Port, _port, optional port number)
        URL_PROPERTY(UString, Path, _path, local path)
        URL_PROPERTY(UString, Query, _query, optional query after '?')
        URL_PROPERTY(UString, Fragment, _fragment, optional fragment after '#')

#undef URL_PROPERTY

        //!
        //! This static method checks if a string contains a URL.
        //! It does not check the full validity of the URL, only if it starts with scheme://.
        //! @param [in] path A file path.
        //! @return True if @a path is a URL.
        //!
        static bool IsURL(const UString& path) { return SchemeLength(path) != 0; }

    private:
        UString  _scheme {};
        UString  _username {};
        UString  _password {};
        UString  _host {};
        uint16_t _port = 0;
        UString  _path {};
        UString  _query {};
        UString  _fragment {};

        // Parse a URL, leave unspecified fields unmodified.
        void parse(const UString& path);

        // Cleanup /../ and /./ from path.
        void cleanupPath();

        // Apply missing base components from a base URL.
        void applyBase(const URL& base);

        // Locate the scheme part of a URL string.
        // Return the size of the leading schem part or zero if there is none.
        static size_t SchemeLength(const UString& path);
    };
}
