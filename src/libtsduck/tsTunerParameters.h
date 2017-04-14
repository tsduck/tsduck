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
//  Abstract base class for DVB tuners parameters
//
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"
#include "tsMPEG.h"
#include "tsModulation.h"
#include "tsTunerArgs.h"
#include "tsSafePtr.h"
#include "tsException.h"

namespace ts {

    // Safe pointer for TunerParameters
    class TunerParameters;
    typedef SafePtr <TunerParameters, Mutex> TunerParametersPtr;

    class TSDUCKDLL TunerParameters: public Object
    {
    public:
        // Get tuner type (depends on subclass)
        TunerType tunerType() const {return _tuner_type;}

        // This abstract method computes the theoretical useful bitrate of a
        // transponder, based on 188-bytes packets, in bits/second.
        // If the characteristics of the transponder are not sufficient
        // to compute the bitrate, return 0.
        virtual BitRate theoreticalBitrate() const = 0;

        // Attempt to convert the tuning parameters in modulation parameters
        // for Dektec modulator cards. This is an optional method. Return true
        // on success, false on error (includes unsupported operation).
        virtual bool convertToDektecModulation (int& modulation_type, int& param0, int& param1, int& param2) const {return false;}

        // Format the tuner parameters according to the Linux DVB "zap" format
        virtual std::string toZapFormat() const = 0;

        // Format the tuner parameters as a list of options for the dvb tsp plugin.
        // If no_local is true, the "local" options are not included.
        // The local options are related to the local equipment (--lnb for instance)
        // and may vary from one system to another for the same transponder.
        virtual std::string toPluginOptions (bool no_local = false) const = 0;

        // Decode a Linux DVB "zap" specification and set the corresponding
        // values in the tuner parameters. Return true on success, false on
        // unsupported format.
        virtual bool fromZapFormat (const std::string& zap) = 0;

        // Return the expected number of fields (separated by ':') in
        // a Linux DVB "zap" specification.
        virtual size_t zapFieldCount () const = 0;

        // Extract options from a TunerArgs, applying defaults when necessary.
        // Return true on success, false on error (missing mandatory parameter,
        // inconsistent values, etc.).
        bool fromTunerArgs (const TunerArgs&, ReportInterface&);

        // Exception thrown when assigning incompatible parameter types
        tsDeclareException (IncompatibleTunerParametersError);

        // Virtual assignment.
        virtual void copy (const TunerParameters&) throw (IncompatibleTunerParametersError) = 0;

        // Virtual destructor
        virtual ~TunerParameters() {}

        // Allocate ("new") a TunerParameters of the appropriate subclass,
        // depending on the tuner type. The parameters have their default values.
        // Return zero if there is no implementation of the parameters for
        // the given tuner type.
        static TunerParameters* Factory (TunerType);

    protected:
        // Set by subclasses
        TunerType _tuner_type;

        // Constructor for subclasses
        TunerParameters (TunerType tuner_type) : _tuner_type (tuner_type) {}

        // Subclass-specific part of fromTunerArgs.
        virtual bool fromArgs (const TunerArgs&, ReportInterface&) = 0;

        // This protected method computes the theoretical useful bitrate of a
        // transponder, based on 188-bytes packets, for QPSK or QAM modulation.
        static BitRate TheoreticalBitrateForModulation (Modulation, InnerFEC, uint32_t symbol_rate);
    };
}
