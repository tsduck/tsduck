//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  General-purpose signalization handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTS.h"
#include "tsPSI.h"

namespace ts {

    class Time;
    class Service;
    // MPEG-defined tables:
    class PAT;
    class CAT;
    class PMT;
    class TSDT;
    // DVB-defined tables:
    class NIT;
    class SDT;
    class BAT;
    class RST;
    class TDT;
    class TOT;
    // ATSC-defined tables:
    class MGT;
    class VCT;
    class CVCT;
    class TVCT;
    class RRT;
    class STT;
    class SAT;

    //!
    //! General-purpose signalization handler interface.
    //! A subclass may choose to implement any handler.
    //! The default implementation of a handler is to do nothing.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL SignalizationHandlerInterface
    {
        TS_INTERFACE(SignalizationHandlerInterface);
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
        //! This hook is invoked when a new UTC time is available.
        //! @param [in] utc The new UTC time.
        //! @param [in] tid The table id into which the time was found.
        //!
        virtual void handleUTC(const Time& utc, TID tid);
        //!
        //! This hook is invoked when a new DVB Satellite Access Table (SAT) is available.
        //! @param [in] table A reference to the new SAT.
        //! @param [in] pid The PID on which the table was found.
        //!
        virtual void handleSAT(const SAT& table, PID pid);
        //!
        //! This hook is invoked when a service in the transport stream has changed.
        //! The change can be minor, such as name or LCN.
        //! @param [in] ts_id The transport stream id or 0xFFFF if it is unknown.
        //! @param [in] service The description of the service. The service id is always set.
        //! Other fields may not be present, check before use.
        //! @param [in] pmt The last PMT of the service. Can be invalid if unknown.
        //! @param [in] removed If true, the service is removed. Otherwise, it is new or changed.
        //!
        virtual void handleService(uint16_t ts_id, const Service& service, const PMT& pmt, bool removed);
    };
}
