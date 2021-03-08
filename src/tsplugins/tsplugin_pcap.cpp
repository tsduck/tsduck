//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//
//  Transport stream processor shared library:
//  Pcap and pcap-ng file input.
//
//----------------------------------------------------------------------------

#include "tsAbstractDatagramInputPlugin.h"
#include "tsPluginRepository.h"
#include "tsPcapFile.h"
#include "tsIPUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class PcapInputPlugin: public AbstractDatagramInputPlugin
    {
        TS_NOBUILD_NOCOPY(PcapInputPlugin);
    public:
        // Implementation of plugin API
        PcapInputPlugin(TSP*);
        virtual bool getOptions() override;
        virtual bool start() override;
        virtual bool stop() override;

    protected:
        // Implementation of AbstractDatagramInputPlugin.
        virtual bool receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp) override;

    private:
        // Command line options:
        UString _file_name;  // Pcap file name.

        // Working data:
        PcapFile    _pcap;          // Pcap file processing.
        MicroSecond _first_tstamp;  // Time stamp of first datagram.
    };
}

TS_REGISTER_INPUT_PLUGIN(u"pcap", ts::PcapInputPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::PcapInputPlugin::PcapInputPlugin(TSP* tsp_) :
    AbstractDatagramInputPlugin(tsp_, IP_MAX_PACKET_SIZE,
                                u"Read TS packets from a pcap or pcap-ng file", u"[options] [file-name]",
                                u"pcap", u"pcap capture time stamp",
                                false), // not real-time network reception
    _file_name(),
    _pcap(),
    _first_tstamp(0)
{
    option(u"", 0, STRING, 0, 1);
    help(u"", u"file-name",
         u"The name of a '.pcap' or '.pcapng' capture file as produced by Wireshark for instance. "
         u"This input plugin extracts IPv4 UDP datagrams which contain transport stream packets. "
         u"Use the standard input by default, when no file name is specified.");
}


//----------------------------------------------------------------------------
// Command line options method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::getOptions()
{
    getValue(_file_name, u"");

    // Get command line arguments for superclass.
    return AbstractDatagramInputPlugin::getOptions();
}


//----------------------------------------------------------------------------
// Start method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::start()
{
    // Initialize superclass and pcap file.
    _first_tstamp = -1;
    return AbstractDatagramInputPlugin::start() && _pcap.open(_file_name, *tsp);
}


//----------------------------------------------------------------------------
// Stop method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::stop()
{
    _pcap.close();
    return AbstractDatagramInputPlugin::stop();
}


//----------------------------------------------------------------------------
// Input method
//----------------------------------------------------------------------------

bool ts::PcapInputPlugin::receiveDatagram(uint8_t* buffer, size_t buffer_size, size_t& ret_size, MicroSecond& timestamp)
{
    // Loop on IPv4 datagrams from the pcap file until a matching UDP packet is found (or end of file).
    for (;;) {

        // Read one IPv4 datagram.
        if (!_pcap.readIPv4(buffer, buffer_size, ret_size, timestamp, *tsp)) {
            return 0;
        }

        // Check that this looks like an IPv4 packet with a UDP transport.
        if (ret_size < IPv4_MIN_HEADER_SIZE || (buffer[0] >> 4) != 4 || buffer[IPv4_PROTOCOL_OFFSET] != IPv4_PROTO_UDP) {
            continue; // not an IPv4 header or not a UDP datagram.
        }
        const size_t ip_header_size = 4 * (buffer[0] & 0x0F);
        if (ip_header_size < IPv4_MIN_HEADER_SIZE || ret_size < ip_header_size + UDP_HEADER_SIZE) {
            continue; // not valid IP + UDP headers.
        }
        const size_t udp_length = GetUInt16BE(buffer + ip_header_size + 4);
        if (udp_length < UDP_HEADER_SIZE || ret_size < ip_header_size + udp_length) {
            continue; // truncated UDP packet.
        }

        // TODO: filter IP addresses, UDP ports. @@@@@

        // Now we have a valid UDP packet. Pack the returned data to remove the IP and UDP headers.
        // Note that memmove() supports overlapping source and destination.
        ret_size = udp_length - UDP_HEADER_SIZE;
        ::memmove(buffer, buffer + ip_header_size + UDP_HEADER_SIZE, ret_size);

        // Adjust time stamps according to first one.
        if (timestamp >= 0) {
            if (_first_tstamp < 0) {
                // This is the first time stamp, the origin.
                _first_tstamp = timestamp;
                timestamp = 0;
            }
            else {
                // Return a relative valud from first timestamp.
                timestamp -= _first_tstamp;
            }
        }

        // Return a valid UDP payload.
        return true;
    }
}
