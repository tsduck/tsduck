//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsMediaTypeUtils.h"


//-----------------------------------------------------------------------------
// General purpose function to delete a heap allocated AM_MEDIA_TYPE structure
// which is useful when calling IEnumMediaTypes::Next as the interface
// implementation allocates the structures which you must later delete
// the format block may also be a pointer to an interface to release.
// Extracted from DirectShow base classes in Windows SDK.
//-----------------------------------------------------------------------------

void ts::DeleteMediaType (::AM_MEDIA_TYPE* pmt)
{
    if (pmt != NULL) {
        ts::FreeMediaType (*pmt);
        ::CoTaskMemFree (pmt);
    }
}


//-----------------------------------------------------------------------------
// Free an existing media type (ie free resources it holds)
//-----------------------------------------------------------------------------

void ts::FreeMediaType (::AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        ::CoTaskMemFree (mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}


//-----------------------------------------------------------------------------
// Copy a media type to another
//-----------------------------------------------------------------------------

::HRESULT ts::CopyMediaType (::AM_MEDIA_TYPE& dst, const ::AM_MEDIA_TYPE& src)
{
    assert (&src != &dst);
    dst = src;
    if (src.cbFormat != 0) {
        assert (src.pbFormat != NULL);
        dst.pbFormat = reinterpret_cast<::BYTE*> (::CoTaskMemAlloc (src.cbFormat));
        if (dst.pbFormat == NULL) {
            dst.cbFormat = 0;
            return E_OUTOFMEMORY;
        }
        else {
            ::CopyMemory(dst.pbFormat, src.pbFormat, dst.cbFormat);  // Flawfinder: ignore: CopyMemory()
        }
    }
    if (dst.pUnk != NULL) {
        dst.pUnk->AddRef();
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// Initialize a media type with "null" values
//-----------------------------------------------------------------------------

void ts::InitMediaType (::AM_MEDIA_TYPE& mt)
{
    mt.majortype = GUID_NULL;
    mt.subtype = GUID_NULL;
    mt.bFixedSizeSamples = 0;
    mt.bTemporalCompression = 0;
    mt.lSampleSize = 0;
    mt.formattype = GUID_NULL;
    mt.pUnk = NULL;
    mt.cbFormat = 0;
    mt.pbFormat = NULL;
}
