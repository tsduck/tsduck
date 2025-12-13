//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  DVB-NIP demux, extracting files and tables.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsmcastFluteDemux.h"
#include "tsmcastNIPHandlerInterface.h"
#include "tsmcastTransportProtocol.h"
#include "tsmcastGatewayConfiguration.h"
#include "tsmcastNetworkInformationFile.h"
#include "tsmcastServiceInformationFile.h"
#include "tsmcastServiceListEntryPoints.h"
#include "tsmcastServiceList.h"
#include "tsmcastNIPService.h"
#include "tsDuckContext.h"
#include "tsIPSocketAddress.h"
#include "tsIPPacket.h"

namespace ts::mcast {
    //!
    //! DVB-NIP demux, extracting files and tables.
    //! @ingroup libtsduck mpeg
    //!
    class TSDUCKDLL NIPDemux : private FluteHandlerInterface
    {
        TS_NOBUILD_NOCOPY(NIPDemux);
    public:
        //!
        //! Constructor.
        //! @param [in,out] duck TSDuck execution context. The reference is kept inside the demux.
        //! @param [in] handler The object to invoke when information is found.
        //!
        explicit NIPDemux(DuckContext& duck, NIPHandlerInterface* handler = nullptr);

        //!
        //! Replace the NIP handler.
        //! @param [in] h The new handler.
        //!
        void setHandler(NIPHandlerInterface* h) { _handler = h; }

        //!
        //! Reset the demux.
        //! @param [in] args Demux arguments.
        //! @return True on success, false on error.
        //!
        bool reset(const FluteDemuxArgs& args);

        //!
        //! The following method feeds the demux with an IP packet.
        //! The packet is ignored if this is not a UDP packet.
        //! @param [in] timestamp Packet time stamp value. This value should be taken from a monotonic clock.
        //! @param [in] pkt An IP packet.
        //!
        template <class Rep, class Period>
        void feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt);

        //!
        //! The following method feeds the demux with a UDP packet.
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
        template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ts::mcast::NIPDemux::ServiceListContext>
        void getServiceLists(CONTAINER& lists) const;
        // Note: Must fully qualify ServiceListContext in same_as<> because of a Visual Studio bug.

        //!
        //! Get a description of all services.
        //! @param [out] services Returned container of service descriptions.
        //! The list of sorted by logical channel number (LCN).
        //!
        template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ts::mcast::NIPService>
        void getServices(CONTAINER& services) const;
        // Note: Must fully qualify NIPService in same_as<> because of a Visual Studio bug.

        //!
        //! Force a file status update in the FLUTE so that the handler can be notified on its handleFluteFile().
        //!
        void getFileStatus() { _flute_demux.getFilesStatus(); }

    private:
        DuckContext&                          _duck;
        Report&                               _report {_duck.report()};
        NIPHandlerInterface*                  _handler = nullptr;
        FluteDemux                            _flute_demux {_duck, this};
        std::set<FluteSessionId>              _session_filter {};
        std::map<UString, ServiceListContext> _service_lists {};  // Service lists, indexed by their URI.
        std::map<UString, NIPService>         _services {};       // Services, indexed by their unique id.

        // Implementation of FluteHandlerInterface.
        // All methods shall be implemented to forward to the NIP handler.
        virtual void handleFluteFile(const FluteFile&) override;
        virtual void handleFluteFDT(const FluteFDT&) override;
        virtual void handleFluteNACI(const NIPActualCarrierInformation&) override;
        virtual void handleFluteStatus(const FluteSessionId&, const UString&, const UString&, uint64_t, uint64_t, uint64_t) override;

        // Process the basic elements of a DVB-NIP stream.
        virtual void processGatewayConfiguration(const GatewayConfiguration& mgc);
        virtual void processNIF(const NetworkInformationFile& nif);
        virtual void processSIF(const ServiceInformationFile& sif);
        virtual void processSLEP(const ServiceListEntryPoints& slep);
        virtual void processServiceList(const ServiceList& service_list);
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

#if !defined(DOXYGEN)

// Feed the analyzer with a UDP packet (IPPacket version).
template <class Rep, class Period>
void ts::mcast::NIPDemux::feedPacket(const cn::duration<Rep,Period>& timestamp, const IPPacket& pkt)
{
    if (pkt.isUDP()) {
        feedPacket(cn::duration_cast<cn::microseconds>(timestamp), pkt.source(), pkt.destination(), pkt.protocolData(), pkt.protocolDataSize());
    }
}

// Feed the analyzer with a UDP packet (IPPacket version).
template <class Rep, class Period>
void ts::mcast::NIPDemux::feedPacket(const cn::duration<Rep,Period>& timestamp, const IPSocketAddress& source, const IPSocketAddress& destination, const uint8_t* udp, size_t udp_size)
{
    // Feed the FLUTE demux with possibly filtered packets. The TSI is not yet accessible, only the addresses.
    if (isFiltered(source, destination)) {
        _flute_demux.feedPacket(timestamp, source, destination, udp, udp_size);
    }
}

// Get a description of all service lists.
template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ts::mcast::NIPDemux::ServiceListContext>
void ts::mcast::NIPDemux::getServiceLists(CONTAINER& lists) const
{
    lists.clear();
    for (const auto& it : _service_lists) {
        lists.push_back(it.second);
        lists.back().file_name = it.first;
    }
}

// Get a description of all services.
template <class CONTAINER> requires std::same_as<typename CONTAINER::value_type, ts::mcast::NIPService>
void ts::mcast::NIPDemux::getServices(CONTAINER& services) const
{
    // Build a temporary multimap, indexed by LCN, to get a sorted list of services.
    std::multimap<uint32_t, const NIPService*> sorted;
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
