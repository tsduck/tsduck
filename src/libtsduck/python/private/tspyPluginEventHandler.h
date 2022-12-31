//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2023, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
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
        //! @ingroup python
        //!
        class TSDUCKDLL PluginEventHandler : public PluginEventHandlerInterface
        {
            TS_NOBUILD_NOCOPY(PluginEventHandler);
        public:
            //!
            //! Profile of a Python callback which receives event.
            //!
            typedef bool (*PyCallback)(uint32_t       event_code,
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
