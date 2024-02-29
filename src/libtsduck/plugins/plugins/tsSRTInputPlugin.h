//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2024, Anthony Delannoy
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Secure Reliable Transport (SRT) input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDatagramInputPlugin.h"
#include "tsSRTSocket.h"

namespace ts {
    //!
    //! Secure Reliable Transport (SRT) input plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL SRTInputPlugin: public AbstractDatagramInputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(SRTInputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool abortInput() override;

    protected:
        // Implementation of AbstractDatagramInputPlugin.
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource) override;

    private:
        SRTSocket _sock {};
    };
}
