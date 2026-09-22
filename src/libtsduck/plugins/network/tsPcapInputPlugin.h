//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Pcap and pcap-ng file input plugin for tsp.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDatagramInputPlugin.h"
#include "tsPcapStream.h"
#include "tsEMMGMUX.h"

namespace ts {
    //!
    //! Pcap and pcap-ng file input plugin for tsp.
    //! @ingroup libtsduck plugin
    //!
    class TSDUCKDLL PcapInputPlugin: public AbstractDatagramInputPlugin
    {
        TS_PLUGIN_CONSTRUCTORS(PcapInputPlugin);
    public:
        // Implementation of plugin API.
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;

    protected:
        // Implementation of AbstractDatagramInputPlugin.
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp, TimeSource& timesource) override;

    private:
        // Command line options:
        fs::path        _file_name {};            // Pcap file name.
        IPSocketAddress _destination {};          // Selected destination UDP socket address.
        IPSocketAddress _source {};               // Selected source UDP socket address.
        bool            _multicast = false;       // Use multicast destinations only.
        bool            _http = false;            // Extract packets from an HTTP session.
        bool            _udp_emmg_mux = false;    // Extract packets from EMMG/PDG <=> MUX data provisions in UDP mode.
        bool            _tcp_emmg_mux = false;    // Extract packets from EMMG/PDG <=> MUX data provisions in TCP mode.
        bool            _has_client_id = false;   // _emmg_client_id is used.
        bool            _has_data_id = false;     // _emmg_data_id is used.
        uint32_t        _emmg_client_id = 0;      // EMMG<=>MUX client id to filter.
        uint16_t        _emmg_data_id = 0;        // EMMG<=>MUX data id to filter.
        size_t          _http_chunk_size = 65535; // Size to load from the TCP session each time we reload the buffer.

        // Working data:
        PcapFilter         _pcap_udp {};          // Pcap file, in UDP mode.
        PcapStream         _pcap_tcp {};          // Pcap file, in TCP mode (DVB SimulCrypt EMMG/PDG <=> MUX).
        cn::microseconds   _first_tstamp {};      // Time stamp of first datagram.
        IPSocketAddress    _actual_dest {};       // Actual destination UDP socket address.
        IPSocketAddress    _actual_source {};     // Actual source TCP socket address for HTTP mode.
        IPSocketAddressSet _all_sources {};       // All source addresses.
        emmgmux::Protocol  _emmgmux {};           // EMMG/PDG <=> MUX protocol instance to decode TCP stream.
        ByteBlock          _data {};              // Session data buffer, for HTTP mode.
        size_t             _data_next = 0;        // Next index in _data.
        bool               _data_error = false;   // Content of _data is invalid.
        bool (PcapInputPlugin::*_receive)(uint8_t*, size_t, size_t&, cn::microseconds&) = nullptr; // Receive handler.

        // Internal receive methods.
        bool receiveUDP(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp);
        bool receiveEMMG(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp);
        bool receiveHTTP(uint8_t* buffer, size_t buffer_size, size_t& ret_size, cn::microseconds& timestamp);

        // Identify and extract TS packets from an EMMG/PDG <=> MUX data_provision message.
        bool isDataProvision(const uint8_t* data, size_t size);
        size_t extractDataProvision(uint8_t* buffer, size_t buffer_size, const uint8_t* msg, size_t msg_size);

        // Report an HTTP content error, make the rest of the stream as invalid.
        void contentErrorHTTP();
    };
}
