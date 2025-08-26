//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Windows module information.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"

namespace ts {
    //!
    //! Windows module information (Windows-specific).
    //! A module is a DLL or an executable file.
    //! @ingroup libtscore windows
    //!
    class TSCOREDLL WinModuleInfo
    {
    public:
        // Description of a Windows module:
        uint64_t file_version_int = 0;      //!< File version as an integer (four 16-bit fields).
        uint64_t product_version_int = 0;   //!< Product version as an integer (four 16-bit fields).
        UString  original_file_name {};     //!< Original_file_name
        UString  file_description {};       //!< File description
        UString  file_version {};           //!< File version
        UString  internal_name {};          //!< Internal name
        UString  product_name {};           //!< Product name
        UString  product_version {};        //!< Product version
        UString  company_name {};           //!< Company name
        UString  legal_copyright {};        //!< Legal copyright
        UString  legal_trademarks {};       //!< Legal trademarks
        UString  private_build {};          //!< Private build
        UString  special_build {};          //!< Special build
        UString  comments {};               //!< Comments

        //!
        //! Default constructor.
        //!
        WinModuleInfo() = default;

        //!
        //! Constructor from a file name.
        //! @param [in] file_name Name of the DLL or executable file.
        //! If no directory is specified, use the LoadLibrary() search algorithm.
        //!
        WinModuleInfo(const UString& file_name);

        //!
        //! Constructor from an address in memory.
        //! @param [in] address Some address in virtual address space, typically a function address.
        //! @param [in] dummy Use nullptr for disambiguation.
        //! Reason: without it, <code>WinModuleInfo(u"foo.dll")</code> would call this constructor,
        //! not the one with a file name.
        //!
        WinModuleInfo(const void* address, std::nullptr_t dummy);

        //!
        //! Clear the content of the structure.
        //!
        void clear();

        //!
        //! Reload content from a file name.
        //! @param [in] file_name Name of the DLL or executable file.
        //! If no directory is specified, use the LoadLibrary() search algorithm.
        //! @return True on success, false on failure.
        //!
        bool reload(const UString& file_name);

        //!
        //! Reload content from an address in memory.
        //! @param [in] address Some address in virtual address space, typically a function address.
        //! @return True on success, false on failure.
        //!
        bool reloadByAddress(const void* address);

        //!
        //! Check if the information was correctly loaded from the module file.
        //! @return True if the information is valid, false otherwise.
        //!
        bool isValid() const { return _last_error.empty(); }

        //!
        //! Get the last error.
        //! @return A constant reference to the last error message.
        //!
        const UString& lastError() const { return _last_error; }

        //!
        //! Get a summary string describing the module.
        //! @return A summary string describing the module.
        //!
        UString summary() const;

        //!
        //! Get a list of Windows-defined names and their corresponding string field in WinModuleInfo.
        //! @return A constant reference on a vector of correspondences.
        //!
        static const std::vector<std::pair<UString WinModuleInfo::*, UString>>& GetNames();

    private:
        UString _last_error {};
    };
}
