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
