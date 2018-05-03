//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2018, Thierry Lelegard
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
//!  A class which scans the services of a transport stream.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsTableHandlerInterface.h"
#include "tsTuner.h"
#include "tsSectionDemux.h"
#include "tsPAT.h"
#include "tsPMT.h"
#include "tsSDT.h"
#include "tsNIT.h"
#include "tsSafePtr.h"
#include "tsCerrReport.h"

namespace ts {
    //!
    //! A class which scans the services of a transport stream.
    //! @ingroup mpeg
    //!
    class TSDUCKDLL TSScanner: private TableHandlerInterface
    {
    public:
        //!
        //! Constructor.
        //! The transport stream is scanned be the constructor.
        //! The collected data cna be fetched later.
        //! @param [in,out] tuner A tuner which is already tuned to the expected channel.
        //! @param [in] timeout Execution timeout in milliseconds.
        //! @param [in] pat_only If true, only collect the PAT, do not wait for more information.
        //! @param [in,out] report Where to report errors.
        //!
        TSScanner(Tuner& tuner, MilliSecond timeout = Infinite, bool pat_only = false, Report& report = CERR);

        //!
        //! Get the list of services.
        //! @param [out] services Returned list of services.
        //! @return True on success, false on error.
        //!
        bool getServices(ts::ServiceList& services) const;

        //!
        //! Get the tuner parameters of the transport stream.
        //! @param [out] tp Returned safe pointer to the tuner parameters.
        //!
        void getTunerParameters(ts::TunerParametersPtr& tp) const {tp = _tparams;}

        //!
        //! Get the PAT of the transport stream.
        //! @param [out] pat Returned safe pointer to the PAT.
        //!
        void getPAT(ts::SafePtr<ts::PAT>& pat) const {pat = _pat;}

        //!
        //! Get the SDT of the transport stream.
        //! @param [out] sdt Returned safe pointer to the SDT.
        //!
        void getSDT(ts::SafePtr<ts::SDT>& sdt) const {sdt = _sdt;}

        //!
        //! Get the NIT of the transport stream.
        //! @param [out] nit Returned safe pointer to the NIT.
        //!
        void getNIT(ts::SafePtr<ts::NIT>& nit) const {nit = _nit;}

    private:
        bool                   _pat_only;
        bool                   _completed;
        ts::Report&   _report;
        ts::SectionDemux       _demux;
        ts::TunerParametersPtr _tparams;
        ts::SafePtr<ts::PAT>   _pat;
        ts::SafePtr<ts::SDT>   _sdt;
        ts::SafePtr<ts::NIT>   _nit;

        // Implementation of TableHandlerInterface.
        virtual void handleTable(ts::SectionDemux&, const ts::BinaryTable&) override;

        // Inaccessible operations.
        TSScanner() = delete;
        TSScanner(const TSScanner&) = delete;
        TSScanner& operator=(const TSScanner&) = delete;
    };
}
