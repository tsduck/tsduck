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
//!  Abstract base class for DVB tuners parameters
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsObject.h"
#include "tsMPEG.h"
#include "tsModulation.h"
#include "tsSafePtr.h"
#include "tsException.h"

namespace ts {

    class TunerArgs;
    class TunerParameters;

    //!
    //! Safe pointer for TunerParameters (thread-safe).
    //!
    typedef SafePtr<TunerParameters, Mutex> TunerParametersPtr;

    //!
    //! Abstract base class for DVB tuners parameters.
    //!
    class TSDUCKDLL TunerParameters: public Object
    {
    public:
        //!
        //! Get the tuner type (depends on subclass).
        //! @return The tuner type.
        //!
        TunerType tunerType() const
        {
            return _tuner_type;
        }

        //!
        //! Theoretical bitrate computation.
        //! Must be implemented by subclasses.
        //! @return The theoretical useful bitrate of a transponder, based
        //! on 188-bytes packets, in bits/second. If the characteristics of
        //! the transponder are not sufficient to compute the bitrate, return 0.
        //!
        virtual BitRate theoreticalBitrate() const = 0;

        //!
        //! Attempt to convert the tuning parameters in modulation parameters for Dektec modulator cards.
        //! This is an optional method.
        //! @param [out] modulation_type Modulation type (DTAPI_MOD_* value).
        //! @param [out] param0 Modulation-specific paramter 0.
        //! @param [out] param1 Modulation-specific paramter 1.
        //! @param [out] param2 Modulation-specific paramter 2.
        //! @return True on success, false on error (includes unsupported operation).
        //!
        virtual bool convertToDektecModulation(int& modulation_type, int& param0, int& param1, int& param2) const
        {
            return false;
        }

        //!
        //! Format a short description (frequency and essential parameters).
        //! @param [in] strength Signal strength in percent. Ignored if negative.
        //! @param [in] quality Signal quality in percent. Ignored if negative.
        //! @return A description string.
        //!
        virtual std::string shortDescription(int strength = -1, int quality = -1) const = 0;

        //!
        //! Format the tuner parameters according to the Linux DVB "zap" format.
        //! @return A string in Linux DVB "zap" format.
        //!
        virtual std::string toZapFormat() const = 0;

        //!
        //! Format the tuner parameters as a list of options for the "dvb" tsp plugin.
        //! @param [in] no_local When true, the "local" options are not included.
        //! The local options are related to the local equipment (--lnb for instance)
        //! and may vary from one system to another for the same transponder.
        //! @return A string containing a command line options for the "dvb" tsp plugin.
        //!
        virtual std::string toPluginOptions(bool no_local = false) const = 0;

        //!
        //! Display a description of the modulation paramters on a stream, line by line.
        //! @param [in,out] strm Where to display the parameters.
        //! @param [in] margin Left margin to display.
        //! @param [in] verbose When false, display only essentials parameters.
        //! When true, display all parameters.
        //!
        virtual void displayParameters(std::ostream& strm, const std::string& margin = std::string(), bool verbose = false) const = 0;

        //!
        //! Decode a Linux DVB "zap" specification.
        //! And set the corresponding values in the tuner parameters.
        //! @param [in] zap A line of a Linux DVB "zap" file.
        //! @return True on success, false on unsupported format.
        //!
        virtual bool fromZapFormat(const std::string& zap) = 0;

        //!
        //! Expected number of fields (separated by ':') in a Linux DVB "zap" specification.
        //! @return Expected number of fields in a Linux DVB "zap" specification.
        //!
        virtual size_t zapFieldCount() const = 0;

        //!
        //! Extract options from a TunerArgs, applying defaults when necessary.
        //! Parameters with irrelevant values (auto, none, etc) are not displayed.
        //! @param [in] args Tuner arguments.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error (missing mandatory parameter,
        //! inconsistent values, etc.).
        //!
        bool fromTunerArgs(const TunerArgs& args, Report& report);

        //!
        //! Exception thrown when assigning incompatible parameter types.
        //!
        TS_DECLARE_EXCEPTION(IncompatibleTunerParametersError);

        //!
        //! Virtual assignment.
        //! @param [in] params Other instance of a subclass of TunerParameters to copy.
        //! @throw IncompatibleTunerParametersError When @a params is from an incompatible type.
        //!
        virtual void copy(const TunerParameters& params) = 0;

        //!
        //! Virtual destructor.
        //!
        virtual ~TunerParameters() {}

        //!
        //! Allocate a TunerParameters of the appropriate subclass, based on a tuner type.
        //! @param [in] type Tuner type.
        //! @return A newly allocated instance of a subclass of TunerParameters.
        //! The parameters have their default values. Return zero if there is no
        //! implementation of the parameters for the given tuner type.
        //!
        static TunerParameters* Factory(TunerType type);

    protected:
        //!
        //! The tuner type is set by subclasses.
        //!
        TunerType _tuner_type;

        //!
        //! Constructor for subclasses.
        //! @param [in] tuner_type Tuner type.
        //!
        TunerParameters(TunerType tuner_type) :
            _tuner_type(tuner_type)
        {
        }

        //!
        //! Subclass-specific part of fromTunerArgs().
        //! @param [in] args Tuner arguments.
        //! @param [in,out] report Where to report errors.
        //! @return True on success, false on error (missing mandatory parameter,
        //! inconsistent values, etc.).
        //!
        virtual bool fromArgs(const TunerArgs& args, Report& report) = 0;

        //!
        //! Theoretical useful bitrate for QPSK or QAM modulation.
        //! This protected static method computes the theoretical useful bitrate of a
        //! transponder, based on 188-bytes packets, for QPSK or QAM modulation.
        //! @param [in] mod Modulation type.
        //! @param [in] fec Inner FEC.
        //! @param [in] symbol_rate Symbol rate.
        //! @return Theoretical useful bitrate in bits/second or zero on error.
        //!
        static BitRate TheoreticalBitrateForModulation(Modulation mod, InnerFEC fec, uint32_t symbol_rate);
    };
}
