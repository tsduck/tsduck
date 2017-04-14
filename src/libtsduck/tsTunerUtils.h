//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//
//  Some utilities for DVB tuners
//
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerParameters.h"
#include "tsDescriptor.h"
#include "tsCerrReport.h"

namespace ts {

    // This method reads a Linux zap configuration file, locate a channel
    // description and sets the TunerParameters to the values for this
    // channel's transponder.
    //
    // Return true on success, false on error.
    //
    // Note: since Linux zap configuration files are text files, they can be
    // used on any platform, although they are usually generated on Linux.

    TSDUCKDLL bool GetTunerFromZapFile (const std::string& channel_name,
                                      const std::string& file_name,
                                      TunerParameters& parameters,
                                      ReportInterface& report = CERR);

    // This method analyzes a delivery system descriptor (satellite,
    // cable or terrestrial) and returns a new tuner parameters object.
    // Return 0 if the descriptor was not correctly analyzed or is not
    // a delivery system descriptor.

    TSDUCKDLL TunerParameters* DecodeDeliveryDescriptor (const Descriptor& desc);
}
