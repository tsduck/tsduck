//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup windows
//!  DirectShow AM_MEDIA_TYPE utilities (Windows-specific).
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsDirectShow.h"

namespace ts {

    //!
    //! Delete a heap-allocated AM_MEDIA_TYPE structure (Windows-specific).
    //! This is useful when calling IEnumMediaTypes::Next as the interface
    //! implementation allocates the structures which you must later delete
    //! the format block may also be a pointer to an interface to release.
    //! @param [in] media_type Media type to delete.
    //!
    TSDUCKDLL void DeleteMediaType(::AM_MEDIA_TYPE* media_type);

    //!
    //! Free an existing media type, ie free resources it holds (Windows-specific).
    //! @param [in] media_type Media type to delete.
    //!
    TSDUCKDLL void FreeMediaType(::AM_MEDIA_TYPE& media_type);

    //!
    //! Copy a media type to another (Windows-specific).
    //! @param [out] dst Destination media type.
    //! @param [in] src Source media type.
    //! @return A COM status code.
    //!
    TSDUCKDLL ::HRESULT CopyMediaType(::AM_MEDIA_TYPE& dst, const ::AM_MEDIA_TYPE& src);

    //!
    //! Initialize a media type with "null" values (Windows-specific).
    //! @param [out] media_type Destination media type.
    //!
    TSDUCKDLL void InitMediaType(::AM_MEDIA_TYPE& media_type);
}
