//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of a media segment in an HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tshlsMediaElement.h"
#include "tsBitRate.h"

namespace ts {
    namespace hls {
        //!
        //! Description of a media segment in an HLS playlist.
        //! @ingroup hls
        //!
        class TSDUCKDLL MediaSegment : public MediaElement
        {
            TS_RULE_OF_FIVE(MediaSegment, override);
        public:
            //!
            //! Constructor.
            //!
            MediaSegment() = default;

            UString     title {};      //!< Optional segment title.
            MilliSecond duration = 0;  //!< Segment duration in milliseconds.
            BitRate     bitrate = 0;   //!< Indicative bitrate.
            bool        gap = false;   //!< Media is a "gap", should not be loaded by clients.
        };
    }
}
