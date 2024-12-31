//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2020-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Reliable Internet Stream Transport (RIST) input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDatagramInputPlugin.h"

namespace ts {
    //!
    //! Reliable Internet Stream Transport (RIST) input plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL RISTInputPlugin: public AbstractDatagramInputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(RISTInputPlugin);
    public:
        //! Destructor.
        virtual ~RISTInputPlugin() override;

        // Implementation of plugin API
        virtual bool getOptions() override;
        virtual bool isRealTime() override;
        virtual bool setReceiveTimeout(cn::milliseconds timeout) override;
        virtual bool start() override;
        virtual bool stop() override;

    protected:
        // Implementation of AbstractDatagramInputPlugin.
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource) override;

    private:
        // The actual implementation is private to the body of the class.
        class Guts;
        Guts* _guts = nullptr;
    };
}
