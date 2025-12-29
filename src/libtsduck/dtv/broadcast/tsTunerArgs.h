//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Parameters for tuners and their command-line definitions.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsModulationArgs.h"
#include "tsTuner.h"

namespace ts {
    //!
    //! Parameters for tuners and their command-line definitions.
    //! @ingroup libtsduck hardware
    //!
    //! All values may be "set" or "unset", depending on command line arguments.
    //! All options for all types of tuners are included here.
    //!
    class TSDUCKDLL TunerArgs : public ModulationArgs
    {
    public:
        // Public fields
        UString device_name {};       //!< Name of tuner device.
        UString receiver_name {};     //!< Name of the DirectShow receiver to use (Windows-specific).
        size_t  demux_buffer_size = Tuner::DEFAULT_DEMUX_BUFFER_SIZE;     //!< Demux buffer size in bytes (Linux-specific).
        size_t  demux_queue_size = Tuner::DEFAULT_SINK_QUEUE_SIZE;        //!< Max number of queued media samples (Windows-specific).
        cn::milliseconds signal_timeout = Tuner::DEFAULT_SIGNAL_TIMEOUT;  //!< Signal locking timeout in milliseconds.
        cn::milliseconds receive_timeout {};                              //!< Packet received timeout in milliseconds.

        //!
        //! Default constructor.
        //! @param [in] info_only If true, the tuner will not be used to tune, just to get information.
        //!
        TunerArgs(bool info_only = false);

        // Overridden from superclass.
        virtual void defineArgs(Args& args, bool allow_short_options) override;
        virtual bool loadArgs(DuckContext& duck, Args& args) override;
        virtual void clear() override;

        //!
        //! Open a tuner and configure it according to the parameters in this object.
        //! @param [in,out] tuner The tuner to open and configure.
        //! @return True on success, false on error.
        //!
        bool configureTuner(Tuner& tuner) const;

    private:
        bool _info_only = false;
    };
}
