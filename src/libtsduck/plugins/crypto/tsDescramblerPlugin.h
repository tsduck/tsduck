//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Generic / sample / reference descrambler plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescrambler.h"
#include "tsDuckProtocol.h"

namespace ts {
    //!
    //! Generic / sample / reference descrambler plugin for tsp.
    //! Can be used as a template for real conditional access systems.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL DescramblerPlugin: public AbstractDescrambler
    {
        TS_PLUGIN_CONSTRUCTORS(DescramblerPlugin);
    public:
        // Implementation of ProcessorPlugin interface.
        virtual bool getOptions() override;

    protected:
        // Implementation of AbstractDescrambler.
        virtual bool checkCADescriptor(CASID pmt_cas_id, const ByteBlock& priv) override;
        virtual bool checkECM(const Section& ecm) override;
        virtual bool decipherECM(const Section& ecm, CWData& cw_even, CWData& cw_odd) override;

    private:
        CASID          _cas_id = 0;
        duck::Protocol _protocol {};
    };
}
