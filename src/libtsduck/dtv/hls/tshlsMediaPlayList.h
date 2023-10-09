//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of a media playlist inside an HLS master playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tshlsMediaElement.h"
#include "tsBitRate.h"

namespace ts {
    namespace hls {
        //!
        //! Description of a media playlist inside an HLS master playlist.
        //! @ingroup hls
        //!
        class TSDUCKDLL MediaPlayList: public MediaElement
        {
        public:
            //!
            //! Constructor.
            //!
            MediaPlayList();

            // Implementation of StringifyInterface
            virtual UString toString() const override;

            // Public fields.
            BitRate bandwidth;        //!< Peak bandwidth.
            BitRate averageBandwidth; //!< Average bandwidth.
            size_t  width;            //!< Resolution width in pixels.
            size_t  height;           //!< Resolution height in pixels.
            size_t  frameRate;        //!< Frame rate in milli-fps.
            UString codecs;           //!< List of codecs.
            UString hdcp;             //!< HDCP level.
            UString videoRange;       //!< Video range description.
            UString video;            //!< Video description.
            UString audio;            //!< Audio description.
            UString subtitles;        //!< Subtitles description.
            UString closedCaptions;   //!< Closed-captions description.
        };
    }
}
