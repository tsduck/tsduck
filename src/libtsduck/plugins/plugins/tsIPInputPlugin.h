//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  IP input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDatagramInputPlugin.h"
#include "tsUDPReceiver.h"

namespace ts {
    //!
    //! IP input plugin for tsp.
    //! @ingroup plugin
    //!
    class TSDUCKDLL IPInputPlugin: public AbstractDatagramInputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(IPInputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;
        virtual bool abortInput() override;
        virtual bool setReceiveTimeout(cn::milliseconds timeout) override;

    protected:
        // Implementation of AbstractDatagramInputPlugin.
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp) override;

    private:
        UDPReceiver _sock; // Incoming socket with associated command line options.
    };
}
