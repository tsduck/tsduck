//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-NIP demux handler interface.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteHandlerInterface.h"

namespace ts::mcast {

    class NetworkInformationFile;
    class ServiceInformationFile;
    class ServiceListEntryPoints;
    class GatewayConfiguration;
    class ServiceList;
    class NIPService;

    //!
    //! DVB-NIP demux handler interface.
    //! @ingroup libtsduck mpeg
    //!
    //! This abstract interface must be implemented by classes which need to be
    //! notified about received files using a NIPDemux. All low-level events from
    //! the FLUTE layer are also available by inheriting from FluteHandlerInterface.
    //!
    class TSDUCKDLL NIPHandlerInterface : public FluteHandlerInterface
    {
        TS_SUBINTERFACE(NIPHandlerInterface);
    public:
        //!
        //! This hook is invoked when a NetworkInformationFile (NIF) is available.
        //! The default implementation does nothing.
        //! @param [in] nif The received file.
        //!
        virtual void handleNetworkInformationFile(const NetworkInformationFile& nif);

        //!
        //! This hook is invoked when a ServiceInformationFile (SIF) is available.
        //! The default implementation does nothing.
        //! @param [in] sif The received file.
        //!
        virtual void handleServiceInformationFile(const ServiceInformationFile& sif);

        //!
        //! This hook is invoked when a multicast gateway configuration is available (including DVB-NIP bootstrap).
        //! The default implementation does nothing.
        //! @param [in] mgc Multicast gateway configuration.
        //!
        virtual void handleGatewayConfiguration(const GatewayConfiguration& mgc);

        //!
        //! This hook is invoked when a DVB-NIP Service List Entry Points (SLEP) is available.
        //! The default implementation does nothing.
        //! @param [in] slep Service list entry points.
        //!
        virtual void handleServiceListEntryPoints(const ServiceListEntryPoints& slep);

        //!
        //! This hook is invoked when a DVB-I or DVB-NIP Service List is available.
        //! The default implementation does nothing.
        //! @param [in] service_list Service list.
        //!
        virtual void handleServiceList(const ServiceList& service_list);

        //!
        //! This hook is invoked when a new service is available.
        //! The default implementation does nothing.
        //! @param [in] service New service.
        //!
        virtual void handleNewService(const NIPService& service);
    };
}
