//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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
//!  Abstract base class for DVB tuners parameters
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"
#include "tsMPEG.h"
#include "tsModulation.h"
#include "tsSafePtr.h"
#include "tsException.h"
#include "tsDescriptor.h"
#include "tsxml.h"

namespace ts {
    //!
    //! Abstract base class for DVB tuners parameters.
    //! @ingroup hardware
    //!
    class TSDUCKDLL TunerParameters: public Object
    {
    public:
        //!
        //! Format a short description (frequency and essential parameters).
        //! @param [in] strength Signal strength in percent. Ignored if negative.
        //! @param [in] quality Signal quality in percent. Ignored if negative.
        //! @return A description string.
        //!
        virtual UString shortDescription(int strength = -1, int quality = -1) const = 0;

        //!
        //! Format the tuner parameters as a list of options for the "dvb" tsp plugin.
        //! @param [in] no_local When true, the "local" options are not included.
        //! The local options are related to the local equipment (--lnb for instance)
        //! and may vary from one system to another for the same transponder.
        //! @return A string containing a command line options for the "dvb" tsp plugin.
        //!
        virtual UString toPluginOptions(bool no_local = false) const = 0;

        //!
        //! Display a description of the modulation paramters on a stream, line by line.
        //! @param [in,out] strm Where to display the parameters.
        //! @param [in] margin Left margin to display.
        //! @param [in] verbose When false, display only essentials parameters.
        //! When true, display all parameters.
        //!
        virtual void displayParameters(std::ostream& strm, const UString& margin = UString(), bool verbose = false) const = 0;
    };
}
