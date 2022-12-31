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
//!  Instanciate all DirectShow devices in a given category.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsComPtr.h"
#include "tsDirectShow.h"

namespace ts {
    //!
    //! A class which instanciates all DirectShow devices in a given category (Windows-specific).
    //! @ingroup windows
    //!
    class TSDUCKDLL DirectShowFilterCategory
    {
        TS_NOBUILD_NOCOPY(DirectShowFilterCategory);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        DirectShowFilterCategory(Report& report);

        //!
        //! Constructor from a device category.
        //! @param [in] category GUID of the device category.
        //! @param [in,out] report Where to report errors.
        //!
        DirectShowFilterCategory(const ::GUID& category, Report& report);

        //!
        //! Destructor.
        //!
        ~DirectShowFilterCategory();

        //!
        //! Build an instance of all filters of the specified category.
        //! @param [in] category GUID of the device category.
        //! @return True on success, false on error.
        //!
        bool getAllFiltersInstance(const ::GUID& category);

        //!
        //! Check if the set of filters is empty.
        //! @return True if there is no filter in the category.
        //!
        bool empty() const
        {
            return _filters.empty();
        }

        //!
        //! Get the number of instantiated devices.
        //! @return The number of instanciated devices.
        //!
        size_t size() const
        {
            return _filters.size();
        }

        //!
        //! Get the name of a device.
        //! @param [in] index Index of the device, from 0 to size()-1.
        //! @return The name of the device or an empty string if @a index is out of range.
        //!
        UString name(size_t index) const
        {
            return index < _filters.size() ? _filters[index].name : UString();
        }

        //!
        //! Get a pointer to the @c IBaseFilter of a device.
        //! @param [in] index Index of the device, from 0 to size()-1.
        //! @return The pointer to the @c IBaseFilter of the device or a null pointer if @a index is out of range.
        //!
        ComPtr<::IBaseFilter> filter(size_t index) const
        {
            return index < _filters.size() ? _filters[index].filter : ComPtr<::IBaseFilter>();
        }

        //!
        //! Clear all devices instances.
        //!
        void clear();

    private:
        // One device entry.
        class Filter
        {
        public:
            ~Filter();      // Destructor.
            void clear();   // Properly clear all fields.

            UString               name;     // Device name.
            ComPtr<::IMoniker>    moniker;  // Moniker to the device object instance.
            ComPtr<::IBaseFilter> filter;   // Pointer to its IBaseFilter interface.
        };

        Report&                  _report;
        ComPtr<::ICreateDevEnum> _enum;
        ComPtr<::IEnumMoniker>   _moniker;
        std::vector<Filter>      _filters;
    };
}
