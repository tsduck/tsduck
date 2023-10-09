//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
