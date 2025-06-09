//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Repository of dynamically registered features.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsNames.h"
#include "tsApplicationSharedLibrary.h"
#include "tsLibTSCoreVersion.h"

namespace ts {
    //!
    //! Repository of dynamically registered features.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL Features
    {
        TS_SINGLETON(Features);
    public:
        //!
        //! Describe the level of support for a feature.
        //!
        enum Support {
            ALWAYS,      //!< Feature is always supported, may ask version but no need to ask if supported.
            SUPPORTED,   //!< Optional feature, currently supported.
            UNSUPPORTED  //!< Optional feature, not supported.
        };

        //!
        //! Each feature is assigned a unique positive number.
        //!
        using index_t = Names::int_t;

        //!
        //! Profile of a function return a version string for a feature.
        //!
        using GetVersionFunc = UString (*)();

        //!
        //! Register a feature.
        //! @param [in] option Feature name as used in command line options.
        //! @param [in] name Feature name as used on display.
        //! @param [in] support Level of support.
        //! @param [in] get_version Function returning the version of the feature. Can be null (no identified version).
        //! @return An index for the feature, as used in isSupported() or getVersion().
        //!
        index_t registerFeature(const UString& option, const UString& name, Support support, GetVersionFunc get_version);

        //!
        //! Register a feature which is in another shared image.
        //! If a feature with the same @a option name is already registered, do nothing.
        //! Otherwise, the @a library name is stored for later usage. The first time the
        //! feature is searched, the shared library is loaded. If the load succeeds and
        //! the initialization of the shared image registered a feature with the same name,
        //! the feature becomes defined. Otherwise, the feature is definitely marked as
        //! unsupported.
        //! @param [in] option Feature name as used in command line options.
        //! @param [in] library Name of a shared image (typically without directory).
        //! @return An index for the feature, as used in isSupported() or getVersion().
        //!
        index_t registerFeature(const UString& option, const fs::path& library);

        //!
        //! A class to register a feature of the application.
        //! The registration is performed using constructors.
        //! Thus, it is possible to perform a registration in the declaration of a static object.
        //!
        class TSCOREDLL Register
        {
            TS_NOBUILD_NOCOPY(Register);
        public:
            //!
            //! Register a feature.
            //! @param [in] option Feature name as used in command line options.
            //! @param [in] name Feature name as used on display.
            //! @param [in] support Level of support.
            //! @param [in] get_version Function returning the version of the feature. Can be null (no identified version).
            //!
            Register(const UString& option, const UString& name, Support support, GetVersionFunc get_version)
            {
                Instance().registerFeature(option, name, support, get_version);
            }

            //!
            //! Register a feature which is in another shared image.
            //! If a feature with the same @a option name is already registered, do nothing.
            //! Otherwise, the @a library name is stored for later usage. The first time the
            //! feature is searched, the shared library is loaded. If the load succeeds and
            //! the initialization of the shared image registered a feature with the same name,
            //! the feature becomes defined. Otherwise, the feature is definitely marked as
            //! unsupported.
            //! @param [in] option Feature name as used in command line options.
            //! @param [in] library Name of a shared image (typically without directory).
            //!
            Register(const UString& option, const fs::path& library)
            {
                Instance().registerFeature(option, library);
            }
        };

        //!
        //! Enumeration of optional features.
        //! Typically used to implement the -\-support command line option.
        //! @return A constant reference to the enumeration description. All integer values are positive.
        //! For a given feature which is both optional and versioned, the same integer value is returned
        //! by supportEnum() and versionEnum().
        //! @see versionEnum()
        //!
        const Names& supportEnum() const { return _support_enum; }

        //!
        //! Enumeration of versioned features.
        //! Typically used to implement the -\-version command line option.
        //! @return A constant reference to the enumeration description. All integer values are positive.
        //! For a given feature which is both optional and versioned, the same integer value is returned
        //! by supportEnum() and versionEnum().
        //! @see supportEnum()
        //!
        const Names& versionEnum() const { return _version_enum; }

        //!
        //! Check if a feature is supported.
        //! @param [in] index Index of the feature as returned in supportEnum().
        //! @return True if the feature is supported.
        //!
        bool isSupported(index_t index);

        //!
        //! Check if a feature is supported.
        //! @param [in] option Command line option of the feature.
        //! @return True if the feature is supported.
        //!
        bool isSupported(const UString& option);

        //!
        //! Get the version of a feature, if supported.
        //! @param [in] index Index of the feature as returned in versionEnum().
        //! @return Feature version string. Empty string if the feature is not supported.
        //!
        UString getVersion(index_t index);

        //!
        //! Get the version of a feature, if supported.
        //! @param [in] option Command line option of the feature.
        //! @return Feature version string. Empty string if the feature is not supported.
        //!
        UString getVersion(const UString& option);

        //!
        //! Get the version of all features.
        //! @return A list of string pairs: feature name, feature version.
        //!
        std::list<std::pair<UString, UString>> getAllVersions();

    private:
        // Description of a feature.
        class TSCOREDLL Feat
        {
            TS_NOCOPY(Feat);
        public:
            // Constructor.
            Feat() = default;

            // Load the shared library if there is one.
            void loadSharedLibrary();

            // Public members.
            UString        option {};
            UString        name {};
            bool           supported = false;
            GetVersionFunc get_version = nullptr;
            fs::path       library_name {};
            std::optional<ApplicationSharedLibrary> library {};
        };

        // Get the description of a feature, nullptr if non-existent.
        const Feat* getFeature(index_t index);

        // Features private members.
        std::map<index_t, Feat> _features {};
        index_t _next_index = 1;
        Names   _support_enum {};
        Names   _version_enum {};
    };
}

//!
//! Registration of a feature for which commands may check support level and version.
//! Same parameters as ts::Features::Register constructors.
//! @hideinitializer
//!
#define TS_REGISTER_FEATURE(...) \
    TS_LIBTSCORE_CHECK(); \
    static ts::Features::Register TS_UNIQUE_NAME(_Registrar)(__VA_ARGS__)
