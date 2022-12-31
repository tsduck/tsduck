//-----------------------------------------------------------------------------
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
