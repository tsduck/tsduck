//----------------------------------------------------------------------------
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
        //! @ingroup plugin
        //!
        //! The input plugin can read HLS playlists and media segments from local
        //! files or receive them in real time using HTTP or HTTPS.
        //!
        class TSDUCKDLL InputPlugin: public AbstractHTTPInputPlugin
        {
            TS_NOBUILD_NOCOPY(InputPlugin);
        public:
            //!
            //! Constructor.
            //! @param [in] tsp Associated callback to @c tsp executable.
            //!
            InputPlugin(TSP* tsp);

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
            URL      _url;
            BitRate  _minRate;
            BitRate  _maxRate;
            size_t   _minWidth;
            size_t   _maxWidth;
            size_t   _minHeight;
            size_t   _maxHeight;
            int      _startSegment;
            bool     _listVariants;
            bool     _lowestRate;
            bool     _highestRate;
            bool     _lowestRes;
            bool     _highestRes;
            size_t   _maxSegmentCount;
            bool     _altSelection;
            UString  _altType;
            UString  _altName;
            UString  _altGroupId;
            UString  _altLanguage;

            // Working data:
            size_t   _segmentCount;
            PlayList _playlist;
        };
    }
}
