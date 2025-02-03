//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
    //! @ingroup libtsduck windows
    //!
    class TSDUCKDLL DirectShowFilterCategory
    {
        TS_NOBUILD_NOCOPY(DirectShowFilterCategory);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        DirectShowFilterCategory(Report& report) : _report(report) {}

        //!
        //! Constructor from a device category.
        //! @param [in] category GUID of the device category.
        //! @param [in,out] report Where to report errors.
        //!
        DirectShowFilterCategory(const ::GUID& category, Report& report);

        //!
        //! Destructor.
        //!
        ~DirectShowFilterCategory() { clear(); }

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
            TS_DEFAULT_COPY_MOVE(Filter);
        public:
            UString               name {};     // Device name.
            ComPtr<::IMoniker>    moniker {};  // Moniker to the device object instance.
            ComPtr<::IBaseFilter> filter {};   // Pointer to its IBaseFilter interface.

            void clear();                      // Properly clear all fields.

            Filter() = default;                // Constructor.
            ~Filter() { clear(); }             // Destructor.
        };

        Report&                  _report;
        ComPtr<::ICreateDevEnum> _enum {};
        ComPtr<::IEnumMoniker>   _moniker {};
        std::vector<Filter>      _filters {};
    };
}
