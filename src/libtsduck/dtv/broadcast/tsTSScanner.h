//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
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
#include "tsLogicalChannelNumbers.h"
#include "tsPAT.h"
#include "tsSDT.h"
#include "tsNIT.h"
#include "tsMGT.h"
#include "tsVCT.h"

namespace ts {
    //!
    //! A class which scans the services of a transport stream.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL TSScanner: private TableHandlerInterface
    {
        TS_NOBUILD_NOCOPY(TSScanner);
    public:
        //!
        //! Constructor.
        //! The transport stream is scanned be the constructor.
        //! The collected data can be fetched later.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the scanner.
        //! @param [in,out] tuner A tuner which is already tuned to the expected channel.
        //! @param [in] timeout Execution timeout in milliseconds.
        //! @param [in] pat_only If true, only collect the PAT, do not wait for more information.
        //!
        TSScanner(DuckContext& duck, Tuner& tuner, cn::milliseconds timeout, bool pat_only = false);

        //!
        //! Get the list of services.
        //! @param [out] services Returned list of services.
        //! @return True on success, false on error.
        //!
        bool getServices(ServiceList& services) const;

        //!
        //! Get the tuner parameters of the transport stream.
        //! @param [out] tp Returned safe pointer to the tuner parameters.
        //!
        void getTunerParameters(ModulationArgs& tp) const { tp = _tparams; }

        //!
        //! Get the PAT of the transport stream.
        //! @param [out] pat Returned safe pointer to the PAT.
        //!
        void getPAT(std::shared_ptr<PAT>& pat) const { pat = _pat; }

        //!
        //! Get the DVB SDT of the transport stream.
        //! @param [out] sdt Returned safe pointer to the DVB SDT.
        //!
        void getSDT(std::shared_ptr<SDT>& sdt) const { sdt = _sdt; }

        //!
        //! Get the DVB NIT of the transport stream.
        //! @param [out] nit Returned safe pointer to the DVB NIT.
        //!
        void getNIT(std::shared_ptr<NIT>& nit) const { nit = _nit; }

        //!
        //! Get the ATSC MGT of the transport stream.
        //! @param [out] mgt Returned safe pointer to the ATSC MGT.
        //!
        void getMGT(std::shared_ptr<MGT>& mgt) const { mgt = _mgt; }

        //!
        //! Get the ATSC VCT of the transport stream.
        //! @param [out] vct Returned safe pointer to the ATSC VCT.
        //! The actual table is eiter a TVCT or a CVCT.
        //!
        void getVCT(std::shared_ptr<VCT>& vct) const { vct = _vct; }

    private:
        DuckContext&   _duck;
        bool           _pat_only = false;
        bool           _completed = false;
        SectionDemux   _demux {_duck, this};
        ModulationArgs _tparams {};
        LogicalChannelNumbers _lcn {_duck};
        std::shared_ptr<PAT>  _pat {};
        std::shared_ptr<SDT>  _sdt {};
        std::shared_ptr<NIT>  _nit {};
        std::shared_ptr<MGT>  _mgt {};
        std::shared_ptr<VCT>  _vct {};

        // Implementation of TableHandlerInterface.
        virtual void handleTable(SectionDemux&, const BinaryTable&) override;
    };
}
