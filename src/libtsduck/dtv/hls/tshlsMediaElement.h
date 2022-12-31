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
//!  Description of a media "element" inside an HLS playlist.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsURL.h"
#include "tsStringifyInterface.h"

namespace ts {
    namespace hls {
        //!
        //! Description of a media "element" (sub-playlist or segment) inside an HLS playlist.
        //! @ingroup hls
        //!
        class TSDUCKDLL MediaElement: public StringifyInterface
        {
        public:
            //!
            //! Constructor.
            //!
            MediaElement();

            //!
            //! Get the URL string to use.
            //! @return The URL string to use.
            //!
            UString urlString() const;

            // Implementation of StringifyInterface
            virtual UString toString() const override;

            // Public fields.
            UString relativeURI;  //!< Relative URI, verbatime from playlist.
            UString filePath;     //!< Full file path.
            URL     url;          //!< Full URL, invalid if accessed by file path only.
        };
    }
}
