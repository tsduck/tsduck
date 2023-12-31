//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command-line options for one plugin.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlugin.h"

namespace ts {
    //!
    //! Command-line options for one plugin.
    //! @ingroup plugin
    //!
    class TSDUCKDLL PluginOptions
    {
    public:
        //!
        //! Default constructor.
        //! @param [in] name Plugin name.
        //! @param [in] args Plugin arguments.
        //!
        PluginOptions(const UString& name = UString(), const UStringVector& args = UStringVector());

        //!
        //! Set values.
        //! @param [in] name Plugin name.
        //! @param [in] args Plugin arguments.
        //!
        void set(const UString& name, const UStringVector& args = UStringVector());

        //!
        //! Clear the content of the options.
        //!
        void clear();

        //!
        //! Format the options as a string.
        //! @param [in] type Plugin type.
        //! @return The command line equivalent.
        //!
        UString toString(PluginType type) const;

        UString       name {};  //!< Plugin name.
        UStringVector args {};  //!< Plugin options.
    };

    //!
    //! A vector of plugin options, representing a processing chain.
    //!
    using PluginOptionsVector = std::vector<PluginOptions>;
}
