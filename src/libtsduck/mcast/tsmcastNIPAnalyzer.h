//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-NIP analyzer.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteDemux.h"
#include "tsmcastTransportProtocol.h"
#include "tsmcastGatewayConfiguration.h"
#include "tsmcastServiceInformationFile.h"
#include "tsmcastServiceListEntryPoints.h"
#include "tsmcastServiceList.h"
#include "tsDuckContext.h"
#include "tsIPSocketAddress.h"
#include "tsIPPacket.h"

namespace ts::mcast {
    //!
    //! DVB-NIP analyzer.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPAnalyzer : protected FluteHandlerInterface
    {
        TS_NOBUILD_NOCOPY(NIPAnalyzer);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. A reference is kept in this object.
        //!
        NIPAnalyzer(DuckContext& duck);

        //!
        //! Reset the analysis.
        //! @param [in] args Analysis arguments.
        //! @return True on success, false on error.
        //!
        virtual bool reset(const FluteDemuxArgs& args);

        //!
        //! The following method feeds the analyzer with an IP packet.
        //! The packet is ignored if this is not a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] pkt An IP packet.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt);

        //!
        //! The following method feeds the analyzer with a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] source Source socket address.
        //! @param [in] destination Destination socket address.
        //! @param [in] udp Address of UDP payload.
        //! @param [in] udp_size Size in bytes of UDP payload.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size);

        //!
        //! Add a FLUTE session in the DVB-NIP analyzer.
        //! There is normally no reason to call this from the application.
        //! The analyzer always starts with the DVB-NIP Announcement Channel on reset().
        //! Then, all declared sessions in the DVB-NIP tables are automatically added.
        //! @param [in] session The session id to add.
        //!
        void addSession(const FluteSessionId& session);

        //!
        //! Add a FLUTE or ROUTE session in the DVB-NIP analyzer.
        //! There is normally no reason to call this from the application.
        //! The analyzer always starts with the DVB-NIP Announcement Channel on reset().
        //! Then, all declared sessions in the DVB-NIP tables are automatically added.
        //! @param [in] protocol The file transport protocol.
        //! @param [in] session The session id to add.
        //!
        void addProtocolSession(const TransportProtocol& protocol, const FluteSessionId& session);

        //!
        //! Check if a UDP packet or FLUTE file is part of a filtered session.
        //! @param [in] source Source IP address.
        //! @param [in] destination Destination IP address and UDP port.
        //! @return True if the packet is part of a filtered session.
        //!
        bool isFiltered(const IPAddress& source, const IPSocketAddress& destination) const;

        //!
        //! Check if a UDP packet or FLUTE file is part of a filtered session.
        //! @param [in] session FLUTE session id.
        //! @return True if the packet is part of a filtered session.
        //!
        bool isFiltered(const FluteSessionId& session) const;

        //!
        //! Description of a DVB-I or DVB-NIP service list.
        //!
        class TSDUCKDLL ServiceListContext
        {
        public:
            ServiceListContext() = default;   //!< Constructor.
            UString        file_name {};      //!< File name of the list in the carousel.
            UString        list_name {};      //!< List title.
            UString        provider_name {};  //!< Provider for the service list.
            FluteSessionId session_id {};     //!< Session where the service list file is received.
        };

        //!
        //! Get a description of all service lists.
        //! @param [out] lists Returned container of service list descriptions.
        //! The list of sorted by file names of service lists.
        //!
        template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ServiceListContext>
        void getServiceLists(CONTAINER& lists) const;

        //!
        //! Description of an instance of service.
        //! A service can be present on severial media.
        //!
        class TSDUCKDLL ServiceInstanceContext
        {
        public:
            ServiceInstanceContext() = default;    //!< Constructor.
            uint32_t       instance_priority = 0;  //!< Priority of this instance.
            UString        media_type {};          //!< MIME type of the media for this instance (HLS playlist, DASH manifest, etc).
            FluteSessionId session_id {};          //!< Session where the service media are received.
        };

        //!
        //! Description of a service.
        //!
        class TSDUCKDLL ServiceContext
        {
        public:
            ServiceContext() = default;         //!< Constructor.
            uint32_t       channel_number = 0;  //!< Logical channel number (LCN).
            bool           selectable = true;   //!< Service is selectable.
            bool           visible = true;      //!< Service is visible.
            UString        service_name {};     //!< Service name.
            UString        provider_name {};    //!< Service provider name.
            std::map<UString, ServiceInstanceContext> instances {}; //!< List of service instances, indexed by media file name.
        };

        //!
        //! Get a description of all services.
        //! @param [out] services Returned container of service descriptions.
        //! The list of sorted by logical channel number (LCN).
        //!
        template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ServiceContext>
        void getServices(CONTAINER& services) const;

    protected:
        //!
        //! This virtual method is invoked for each multicast gateway configuration (including DVB-NIP bootstrap).
        //! When overridden in a subclass, call the superclass first.
        //! @param [in] mgc Multicast gateway configuration.
        //!
        virtual void processGatewayConfiguration(const GatewayConfiguration& mgc);

        //!
        //! This virtual method is invoked for each DVB-NIP Service Information File (SIF).
        //! When overridden in a subclass, call the superclass first.
        //! @param [in] sif Service information file.
        //!
        virtual void processSIF(const ServiceInformationFile& sif);

        //!
        //! This virtual method is invoked for each DVB-NIP Service List Entry Points (SLEP).
        //! When overridden in a subclass, call the superclass first.
        //! @param [in] slep Service list entry points.
        //!
        virtual void processSLEP(const ServiceListEntryPoints& slep);

        //!
        //! This virtual method is invoked for each DVB-MABR Service List.
        //! When overridden in a subclass, call the superclass first.
        //! @param [in] service_list Service list.
        //!
        virtual void processServiceList(const ServiceList& service_list);

        //!
        //! This virtual method is invoked for each new service.
        //! When overridden in a subclass, call the superclass first.
        //! @param [in] service New service.
        //!
        virtual void processNewService(const ServiceContext& service);

    private:
        // NIPAnalyzer private fields.
        DuckContext& _duck;
        Report&      _report {_duck.report()};
        FluteDemux   _flute_demux {_duck, this};
        std::set<FluteSessionId>              _session_filter {};
        std::map<UString, ServiceListContext> _service_lists {};  // Service lists, indexed by their URI.
        std::map<UString, ServiceContext>     _services {};       // Services, indexed by their unique id.

        // Inherited methods.
        virtual void handleFluteFile(const FluteFile&) override;
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Feed the analyzer with a UDP packet (IPPacket version).
template <class Rep, class Period>
void ts::mcast::NIPAnalyzer::feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacket(cn::duration_cast<cn::microseconds>(timestamp), pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}

// Feed the analyzer with a UDP packet (IPPacket version).
template <class Rep, class Period>
void ts::mcast::NIPAnalyzer::feedPacket(const cn::duration<Rep,Period>& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Feed the FLUTE demux with possibly filtered packets. The TSI is not yet accessible, only the addresses.
    if (isFiltered(source, destination)) {
        _flute_demux.feedPacket(timestamp, source, destination, udp, udp_size);
    }
}

// Get a description of all service lists.
template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ts::mcast::NIPAnalyzer::ServiceListContext>
void ts::mcast::NIPAnalyzer::getServiceLists(CONTAINER& lists) const
{
    lists.clear();
    for (const auto& it : _service_lists) {
        lists.push_back(it.second);
        lists.back().file_name = it.first;
    }
}

// Get a description of all services.
template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ts::mcast::NIPAnalyzer::ServiceContext>
void ts::mcast::NIPAnalyzer::getServices(CONTAINER& services) const
{
    // Build a temporary multimap, indexed by LCN, to get a sorted list of services.
    std::multimap<uint32_t, const ServiceContext*> sorted;
    for (const auto& it1 : _services) {
        sorted.insert(std::make_pair(it1.second.channel_number, &it1.second));
    }

    // Use the sorted map to return the list of services.
    services.clear();
    for (const auto& it : sorted) {
        services.push_back(*it.second);
    }
}

#endif // DOXYGEN
