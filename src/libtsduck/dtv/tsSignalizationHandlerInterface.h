//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
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
//!  General-purpose signalization handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
// MPEG-defined tables:
#include "tsPAT.h"
#include "tsCAT.h"
#include "tsPMT.h"
#include "tsTSDT.h"
// DVB-defined tables:
#include "tsNIT.h"
#include "tsSDT.h"
#include "tsBAT.h"
#include "tsRST.h"
#include "tsTDT.h"
#include "tsTOT.h"
// ATSC-defined tables:
#include "tsMGT.h"
#include "tsCVCT.h"
#include "tsTVCT.h"
#include "tsRRT.h"
#include "tsSTT.h"

namespace ts {
    //!
    //! General-purpose signalization handler interface.
    //! A subclass may choose to implement any handler.
    //! The default implementation of a handler is to do nothing.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SignalizationHandlerInterface
    {
    public:
        //!
        //! This hook is invoked when a new MPEG Program Association Table (PAT) is available.
        //! @param [in] table A reference to the new PAT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handlePAT(const PAT& table, PID pid);
        //!
        //! This hook is invoked when a new MPEG Conditional Access Table (CAT) is available.
        //! @param [in] table A reference to the new CAT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleCAT(const CAT& table, PID pid);
        //!
        //! This hook is invoked when a new MPEG Program Map Table (PMT) is available.
        //! @param [in] table A reference to the new PMT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handlePMT(const PMT& table, PID pid);
        //!
        //! This hook is invoked when a new MPEG Transport Stream Description Table (TSDT) is available.
        //! @param [in] table A reference to the new TSDT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleTSDT(const TSDT& table, PID pid);
        //!
        //! This hook is invoked when a new DVB Network Information Table (NIT) is available.
        //! @param [in] table A reference to the new NIT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleNIT(const NIT& table, PID pid);
        //!
        //! This hook is invoked when a new DVB Service Description Table (SDT) is available.
        //! @param [in] table A reference to the new SDT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleSDT(const SDT& table, PID pid);
        //!
        //! This hook is invoked when a new DVB Bouquet Association Table (BAT) is available.
        //! @param [in] table A reference to the new BAT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleBAT(const BAT& table, PID pid);
        //!
        //! This hook is invoked when a new DVB Running Status Table (RST) is available.
        //! @param [in] table A reference to the new RST.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleRST(const RST& table, PID pid);
        //!
        //! This hook is invoked when a new DVB Time & Date Table (TDT) is available.
        //! @param [in] table A reference to the new TDT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleTDT(const TDT& table, PID pid);
        //!
        //! This hook is invoked when a new DVB Time Offset Table (TOT) is available.
        //! @param [in] table A reference to the new TOT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleTOT(const TOT& table, PID pid);
        //!
        //! This hook is invoked when a new ATSC Master Guide Table (MGT) is available.
        //! @param [in] table A reference to the new MGT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleMGT(const MGT& table, PID pid);
        //!
        //! This hook is invoked when a new ATSC Virtual Channel Table (VCT) is available.
        //! @param [in] table A reference to the new VCT. This can be a CVCT or a TVCT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleVCT(const VCT& table, PID pid);
        //!
        //! This hook is invoked when a new ATSC Cable Virtual Channel Table (CVCT) is available.
        //! Note that handleVCT() and handleCVCT() are sequentially invoked for each CVCT.
        //! @param [in] table A reference to the new CVCT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleCVCT(const CVCT& table, PID pid);
        //!
        //! This hook is invoked when a new ATSC Terrestrial Virtual Channel Table (TVCT) is available.
        //! Note that handleVCT() and handleTVCT() are sequentially invoked for each TVCT.
        //! @param [in] table A reference to the new TVCT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleTVCT(const TVCT& table, PID pid);
        //!
        //! This hook is invoked when a new ATSC Rating Region Table (RRT) is available.
        //! @param [in] table A reference to the new RRT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleRRT(const RRT& table, PID pid);
        //!
        //! This hook is invoked when a new ATSC System Time Table (STT) is available.
        //! @param [in] table A reference to the new STT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleSTT(const STT& table, PID pid);
        //!
        //! Virtual destructor.
        //!
        virtual ~SignalizationHandlerInterface() = default;
    };
}
