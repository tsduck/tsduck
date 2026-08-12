//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  HLS input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractHTTPInputPlugin.h"
#include "tshlsPlayList.h"
#include "tsURL.h"

namespace ts {
    namespace hls {
        //!
        //! HTTP Live Streaming (HLS) input plugin for tsp.
        //! @ingroup libtsduck plugin
        //!
        //! The input plugin can read HLS playlists and media segments from local
        //! files or receive them in real time using HTTP or HTTPS.
        //!
        class TSDUCKDLL InputPlugin: public AbstractHTTPInputPlugin
        {
            TS_PLUGIN_CONSTRUCTORS(InputPlugin);
        public:
            // Implementation of plugin API
            virtual bool getOptions() override;
            virtual bool start() override;
            virtual bool stop() override;
            virtual bool isRealTime() override;

        protected:
            // Implementation of AbstractHTTPInputPlugin
            virtual bool openURL(WebRequest&) override;

        private:
            // Command line options:
            URL      _url {};
            BitRate  _min_rate = 0;
            BitRate  _max_rate = 0;
            size_t   _min_width = 0;
            size_t   _max_width = 0;
            size_t   _min_height = 0;
            size_t   _max_height = 0;
            int      _start_segment = 0;
            bool     _list_variants = false;
            bool     _lowest_rate = false;
            bool     _highest_rate = false;
            bool     _lowest_res = false;
            bool     _highest_res = false;
            size_t   _max_segment_count = 0;
            bool     _alt_selection = false;
            UString  _alt_type {};
            UString  _alt_name {};
            UString  _alt_group_id {};
            UString  _alt_language {};

            // Working data:
            size_t   _segment_count = 0;
            PlayList _playlist {};
        };
    }
}
