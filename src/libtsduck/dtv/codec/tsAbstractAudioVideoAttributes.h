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
//!  Abstract base class for MPEG audio and video attributes
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"
#include "tsStringifyInterface.h"

namespace ts {

    //!
    //! Abstract base class for MPEG audio and video attributes
    //! @ingroup mpeg
    //!
    class TSDUCKDLL AbstractAudioVideoAttributes: public StringifyInterface
    {
    public:
        //!
        //! Default constructor
        //!
        AbstractAudioVideoAttributes();

        //!
        //! Virtual destructor
        //!
        virtual ~AbstractAudioVideoAttributes() override;

        //!
        //! Check if the values in the object are valid.
        //! @return True if the values in the object are valid.
        //!
        bool isValid() const {return _is_valid;}

        //!
        //! Invalidate the content of this instance.
        //! It must be rebuilt using audio/video binary data.
        //!
        void invalidate() {_is_valid = false;}

        //!
        //! Provides an audio/video binary data to be analyzed by this instance.
        //! The type of data (complete PES payload or some type of "frame" or "access unit")
        //! depends on the type of audio/video.
        //! @param [in] addr Address of data to be analyzed.
        //! @param [in] size Size of data to be analyzed.
        //! @return True if the attributes object becomes valid or has new values.
        //!
        virtual bool moreBinaryData(const uint8_t* addr, size_t size) = 0;

    protected:
        //!
        //! A flag which indicates if the content of this object is valid.
        //! It is the responsibility of the subclasses to set it.
        //!
        bool _is_valid;
    };
}
