//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
        TS_RULE_OF_FIVE(AbstractAudioVideoAttributes, override);
    public:
        //!
        //! Default constructor
        //!
        AbstractAudioVideoAttributes() = default;

        //!
        //! Check if the values in the object are valid.
        //! @return True if the values in the object are valid.
        //!
        bool isValid() const { return _is_valid; }

        //!
        //! Invalidate the content of this instance.
        //! It must be rebuilt using audio/video binary data.
        //!
        void invalidate() { _is_valid = false; }

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
        bool _is_valid = false;
    };
}
