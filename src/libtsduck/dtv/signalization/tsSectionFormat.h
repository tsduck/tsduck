//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of the storage formats for PSI/SI sections and tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"

namespace ts {
    //!
    //! Storage formats for PSI/SI sections and tables.
    //!
    enum class SectionFormat {
        UNSPECIFIED,  //!< Unspecified, depends on context, such as file name extension.
        BINARY,       //!< Binary sections.
        XML,          //!< XML tables representation.
        JSON,         //!< JSON (translated XML) tables representation.
    };


    //!
    //! Enumeration description of ts::SectionFormat.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Enumeration& SectionFormatEnum();

    //!
    //! Enumeration description of ts::SectionFormat, excluding ts::SectionFormat::UNSPECIFIED.
    //! Useful to declare a parameter value which needs a specific format.
    //! @return A constant reference to the enumeration description.
    //!
    TSDUCKDLL const Enumeration& SpecifiedSectionFormatEnum();

    //!
    //! Get a section file type, based on a file name.
    //! @param [in] file_name File name or inline XML or inline JSON.
    //! @param [in] type File type.
    //! @return If @a type is not SectionFormat::UNSPECIFIED, return @a type.
    //! Otherwise, return the file type based on the file name. If the file
    //! name has no known extension, return SectionFormat::UNSPECIFIED.
    //!
    TSDUCKDLL SectionFormat GetSectionFileFormat(const UString& file_name, SectionFormat type = SectionFormat::UNSPECIFIED);

    //!
    //! Build a section file name, based on a file type.
    //! @param [in] file_name File name.
    //! @param [in] type File type.
    //! @return If @a type is not SectionFormat::UNSPECIFIED, remove the
    //! extension from @a file_name and add the extension corresponding to @a type.
    //!
    TSDUCKDLL fs::path BuildSectionFileName(const fs::path& file_name, SectionFormat type);

    //!
    //! Default file name suffix for binary section files.
    //!
    constexpr const UChar* DEFAULT_BINARY_FILE_SUFFIX = u".bin";

    //!
    //! Default file name suffix for XML section files.
    //!
    constexpr const UChar* DEFAULT_XML_FILE_SUFFIX = u".xml";

    //!
    //! Default file name suffix for JSON section files.
    //!
    constexpr const UChar* DEFAULT_JSON_FILE_SUFFIX = u".json";
}
