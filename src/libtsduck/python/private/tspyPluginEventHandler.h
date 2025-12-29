//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Plugin event handler with notification to a Python class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPluginEventHandlerInterface.h"

namespace ts {
    namespace py {
        //!
        //! Plugin event handler with notification to a Python class.
        //! @ingroup libtsduck python
        //!
        class TSDUCKDLL PluginEventHandler : public PluginEventHandlerInterface
        {
            TS_NOBUILD_NOCOPY(PluginEventHandler);
        public:
            //!
            //! Profile of a Python callback which receives event.
            //!
            using PyCallback = bool (*)(uint32_t       event_code,
                                        const UChar*   plugin_name_addr,
                                        size_t         plugin_name_bytes,
                                        size_t         plugin_index,
                                        size_t         plugin_count,
                                        size_t         bitrate,
                                        size_t         plugin_packets,
                                        size_t         total_packets,
                                        const uint8_t* data_addr,
                                        size_t         data_size,
                                        size_t         data_max_size,
                                        bool           data_read_only,
                                        void*          event_data);

            //!
            //! Constructor.
            //! @param [in] callback Python callback to receive events.
            //!
            PluginEventHandler(PyCallback callback);

            //!
            //! Destructor.
            //!
            virtual ~PluginEventHandler() override;

        private:
            PyCallback _callback;

            // Inherited from ts::PluginEventHandlerInterface:
            virtual void handlePluginEvent(const PluginEventContext& context) override;
        };
    }
}
