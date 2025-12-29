//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  General-purpose plugin event data referencing binary data.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"

namespace ts {
    //!
    //! General-purpose plugin event data referencing binary data to exchange with applications.
    //! @ingroup libtsduck plugin
    //!
    //! This subclass of Object can be used as "plugin data" when a plugin triggers
    //! an event and wants to pass to the application a read-only binary area.
    //!
    //! The plugin event handlers in the application are synchronously invoked in
    //! the context of the thread plugin. The referenced binary data can be local
    //! data inside the plugin. The event handler may not saved a reference to it.
    //!
    class TSDUCKDLL PluginEventData : public Object
    {
        // Prevent copy to allow safe storage of references.
        TS_NOBUILD_NOCOPY(PluginEventData);
    public:
        //!
        //! Constructor passing read-only event data.
        //! @param [in] data Address of the plugin event data to pass to applications. It can be a null pointer.
        //! @param [in] size Size in bytes of the plugin data.
        //!
        PluginEventData(const uint8_t* data, size_t size);

        //!
        //! Constructor passing read-write event data.
        //! @param [in] data Address of the plugin event data to pass to applications. It can be a null pointer.
        //! @param [in] size Initial size in bytes of the plugin event data.
        //! @param [in] max_size Maximum size in bytes of the plugin event data buffer. It must not be less than @a size.
        //! If the application modifies the data, it shall not write more than @a max_size bytes.
        //!
        PluginEventData(uint8_t* data, size_t size, size_t max_size);

        //!
        //! Destructor.
        //!
        virtual ~PluginEventData() override;

        //!
        //! Check if the plugin event data area is read-only.
        //! @return True if the plugin event data area is read-only.
        //!
        bool readOnly() const { return _read_only; }

        //!
        //! Get the address of the plugin read-only event data.
        //! @return The address of the plugin event data.
        //!
        const uint8_t* data() const { return _data; }

        //!
        //! Get the current size in bytes of the plugin event data.
        //! If the event data is modifiable, this may change.
        //! @return The current size in bytes of the plugin data.
        //!
        size_t size() const { return _cur_size; }

        //!
        //! Get the maximum size in bytes of the plugin event data.
        //! If the event data is modifiable, this may be more than size().
        //! @return The maximum size in bytes of the plugin data.
        //!
        size_t maxSize() const { return _max_size; }

        //!
        //! Get the remaining modifiable size in bytes of the plugin event data.
        //! If the event data is modifiable, this is zero.
        //! @return The remaining modifiable size in bytes of the plugin data.
        //!
        size_t remainingSize() const { return _max_size - _cur_size; }

        //!
        //! Append new application data inside the plugin event data area.
        //! The plugin event data area must not
        //! @param [in] data_addr Address of the data to append at the end of the plugin event data.
        //! @param [in] data_size Size in bytes of the data to append.
        //! @return True if the data were copied. False if the data area is read-only of the specified
        //! data are too large.
        //!
        bool append(const void* data_addr, size_t data_size);

        //!
        //! Get the address of the plugin modifiable event data.
        //! When the event data are not read-only, the application may update them
        //! directly, within the limits of maxSize().
        //! @return The address of the plugin event data or the null pointer if the event data area is read-only.
        //!
        uint8_t* outputData() const { return _read_only ? nullptr : _data; }

        //!
        //! Update the current size of the plugin modifiable event data.
        //! When the event data are not read-only, the application may update the event data
        //! directly, within the limits of maxSize(). This method shall be used if the current
        //! data size is changed.
        //! @param [in] size The new event data size in bytes. Must not be more than maxSize().
        //! @return True on success, false if the event data are read-only or @a size is too large.
        //!
        bool updateSize(size_t size);

        //!
        //! Set the error indicator in the event data.
        //! @param [in] error Error indicator (default is true).
        //!
        void setError(bool error = true) { _error = error; }

        //!
        //! Check the error indicator in the event data.
        //! @return True if an event handler has reported an error.
        //!
        bool hasError() const { return _error; }

    private:
        bool     _read_only = true;
        bool     _error = false;
        uint8_t* _data = nullptr;
        size_t   _max_size = 0;
        size_t   _cur_size = 0;
    };
}
