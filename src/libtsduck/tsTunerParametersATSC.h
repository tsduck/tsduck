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
//!
//!  @file
//!  ATSC (terrestrial, cable) tuners parameters
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTunerParameters.h"

namespace ts {
    //!
    //! ATSC (terrestrial, cable) tuners parameters
    //!
    class TSDUCKDLL TunerParametersATSC: public TunerParameters
    {
    public:
        // Public fields
        uint64_t          frequency;  //!< Carrier frequency, in Hz.
        SpectralInversion inversion;  //!< Spectral inversion, should be SPINV_AUTO.
        Modulation        modulation; //!< Modulation type.

        // Default values
        static const SpectralInversion DEFAULT_INVERSION  = SPINV_AUTO;  //!< Default value for inversion.
        static const Modulation        DEFAULT_MODULATION = VSB_8;       //!< Default value formodulation.

        //!
        //! Default constructor.
        //!
        TunerParametersATSC();

        // Implementation of TunerParameters API
        virtual BitRate theoreticalBitrate() const {return 0;} // unknown for ATSC/VSB
        virtual std::string shortDescription(int strength = -1, int quality = -1) const;
        virtual std::string toZapFormat() const {return "";} //TODO: unimplemented
        virtual std::string toPluginOptions(bool no_local = false) const;
        virtual void displayParameters(std::ostream& strm, const std::string& margin = std::string(), bool verbose = false) const;
        virtual bool fromZapFormat(const std::string& zap) {return false;} //TODO: unimplemented
        virtual size_t zapFieldCount() const {return 0;} //TODO: unimplemented
        virtual void copy(const TunerParameters&);
    protected:
        virtual bool fromArgs(const TunerArgs&, ReportInterface&);
    };
}
