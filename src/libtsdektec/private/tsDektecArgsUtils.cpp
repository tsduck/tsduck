//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------

#include "tsDektecArgsUtils.h"
#include "tsIPSocketAddress.h"
#include "tsMemory.h"


//-----------------------------------------------------------------------------
// Embed DTAPI SetIoConfig parameters Value and SubValue in one int.
//-----------------------------------------------------------------------------

namespace {
    int IoConfigParams(int value, int subvalue = -1) { return (value & 0xFFFF) | (subvalue << 16); }
    int IoConfigValue(int opt) { return opt & 0xFFFF; }
    int IoConfigSubValue(int opt) { const int o = (opt >> 16) & 0xFFFF; return o == 0xFFFF ? -1 : o; }
}


//-----------------------------------------------------------------------------
// Add command line option definitions in an Args for --io-standard option.
//-----------------------------------------------------------------------------

void ts::DefineDektecIOStandardArgs(Args& args)
{
    args.option(u"io-standard", 0, Names({
        {u"ASI",                  IoConfigParams(DTAPI_IOCONFIG_ASI)},                                 // DVB-ASI transport stream
        {u"SPI",                  IoConfigParams(DTAPI_IOCONFIG_SPI)},                                 // DVB-SPI transport stream
        {u"IF-AD-converter",      IoConfigParams(DTAPI_IOCONFIG_IFADC)},                               // IF A/D converter
        {u"IP",                   IoConfigParams(DTAPI_IOCONFIG_IP)},                                  // Transport stream over IP
        {u"dektec-streaming",     IoConfigParams(DTAPI_IOCONFIG_DEKTECST)},                            // DekTec Streaming-data Interface
        {u"demodulator",          IoConfigParams(DTAPI_IOCONFIG_DEMOD)},                               // Demodulation
        {u"modulator",            IoConfigParams(DTAPI_IOCONFIG_MOD)},                                 // Modulator output
        /*
         * The following modes used to be defined, but they do not support MPEG-TS format.
         * As a consequence, they are now disabled.
         *
        {u"12G-SDI-2160p/50",     IoConfigParams(DTAPI_IOCONFIG_12GSDI, DTAPI_IOCONFIG_2160P50)},      // 12G-SDI, 2160p/50 lvl A
        {u"12G-SDI-2160p/50B",    IoConfigParams(DTAPI_IOCONFIG_12GSDI, DTAPI_IOCONFIG_2160P50B)},     // 12G-SDI, 2160p/50 lvl B
        {u"12G-SDI-2160p/59.94",  IoConfigParams(DTAPI_IOCONFIG_12GSDI, DTAPI_IOCONFIG_2160P59_94)},   // 12G-SDI, 2160p/59.94 lvl A
        {u"12G-SDI-2160p/59.94B", IoConfigParams(DTAPI_IOCONFIG_12GSDI, DTAPI_IOCONFIG_2160P59_94B)},  // 12G-SDI, 2160p/59.94 lvl B
        {u"12G-SDI-2160p/60",     IoConfigParams(DTAPI_IOCONFIG_12GSDI, DTAPI_IOCONFIG_2160P60)},      // 12G-SDI, 2160p/60 lvl A
        {u"12G-SDI-2160p/60B",    IoConfigParams(DTAPI_IOCONFIG_12GSDI, DTAPI_IOCONFIG_2160P60B)},     // 12G-SDI, 2160p/60 lvl B
        {u"3G-SDI-1080p/50",      IoConfigParams(DTAPI_IOCONFIG_3GSDI, DTAPI_IOCONFIG_1080P50)},       // 3G-SDI, 1080p/50 lvl A
        {u"3G-SDI-1080p/50B",     IoConfigParams(DTAPI_IOCONFIG_3GSDI, DTAPI_IOCONFIG_1080P50B)},      // 3G-SDI, 1080p/50 lvl B
        {u"3G-SDI-1080p/59.94",   IoConfigParams(DTAPI_IOCONFIG_3GSDI, DTAPI_IOCONFIG_1080P59_94)},    // 3G-SDI, 1080p/59.94 lvl A
        {u"3G-SDI-1080p/59.94B",  IoConfigParams(DTAPI_IOCONFIG_3GSDI, DTAPI_IOCONFIG_1080P59_94B)},   // 3G-SDI, 1080p/59.94 lvl B
        {u"3G-SDI-1080p/60",      IoConfigParams(DTAPI_IOCONFIG_3GSDI, DTAPI_IOCONFIG_1080P60)},       // 3G-SDI, 1080p/60 lvl A
        {u"3G-SDI-1080p/60B",     IoConfigParams(DTAPI_IOCONFIG_3GSDI, DTAPI_IOCONFIG_1080P60B)},      // 3G-SDI, 1080p/60 lvl B
        {u"6G-SDI-2160p/23.98",   IoConfigParams(DTAPI_IOCONFIG_6GSDI, DTAPI_IOCONFIG_2160P23_98)},    // 6G-SDI, 2160p/23.98
        {u"6G-SDI-2160p/24",      IoConfigParams(DTAPI_IOCONFIG_6GSDI, DTAPI_IOCONFIG_2160P24)},       // 6G-SDI, 2160p/24
        {u"6G-SDI-2160p/25",      IoConfigParams(DTAPI_IOCONFIG_6GSDI, DTAPI_IOCONFIG_2160P25)},       // 6G-SDI, 2160p/25
        {u"6G-SDI-2160p/29.97",   IoConfigParams(DTAPI_IOCONFIG_6GSDI, DTAPI_IOCONFIG_2160P29_97)},    // 6G-SDI, 2160p/29.97
        {u"6G-SDI-2160p/30",      IoConfigParams(DTAPI_IOCONFIG_6GSDI, DTAPI_IOCONFIG_2160P30)},       // 6G-SDI, 2160p/30
        {u"encoder",              IoConfigParams(DTAPI_IOCONFIG_AVENC)},                               // Audio/video encoder
        {u"GPS-clock",            IoConfigParams(DTAPI_IOCONFIG_GPSTIME)},                             // 1 PPS and 10 MHz GPS-clock input
        {u"HDMI",                 IoConfigParams(DTAPI_IOCONFIG_HDMI)},                                // HDMI
        {u"HD-SDI-1080i/50",      IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080I50)},       // HD-SDI, 1080i/50
        {u"HD-SDI-1080i/59.94",   IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080I59_94)},    // HD-SDI, 1080i/59.94
        {u"HD-SDI-1080i/60",      IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080I60)},       // HD-SDI, 1080i/60
        {u"HD-SDI-1080p/23.98",   IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080P23_98)},    // HD-SDI, 1080p/23.98
        {u"HD-SDI-1080p/24",      IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080P24)},       // HD-SDI, 1080p/24
        {u"HD-SDI-1080p/25",      IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080P25)},       // HD-SDI, 1080p/25
        {u"HD-SDI-1080p/29.97",   IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080P29_97)},    // HD-SDI, 1080p/29.97
        {u"HD-SDI-1080p/30",      IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080P30)},       // HD-SDI, 1080p/30
        {u"HD-SDI-1080psf/23.98", IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080PSF23_98)},  // HD-SDI, 1080psf/23.98
        {u"HD-SDI-1080psf/24",    IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080PSF24)},     // HD-SDI, 1080psf/24
        {u"HD-SDI-1080psf/25",    IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080PSF25)},     // HD-SDI, 1080psf/25
        {u"HD-SDI-1080psf/29.97", IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080PSF29_97)},  // HD-SDI, 1080psf/29.97
        {u"HD-SDI-1080psf/30",    IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_1080PSF30)},     // HD-SDI, 1080psf/30
        {u"HD-SDI-720p/23.98",    IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P23_98)},     // HD-SDI, 720p/23.98
        {u"HD-SDI-720p/24",       IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P24)},        // HD-SDI, 720p/24
        {u"HD-SDI-720p/25",       IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P25)},        // HD-SDI, 720p/25
        {u"HD-SDI-720p/29.97",    IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P29_97)},     // HD-SDI, 720p/29.97
        {u"HD-SDI-720p/30",       IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P30)},        // HD-SDI, 720p/30
        {u"HD-SDI-720p/50",       IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P50)},        // HD-SDI, 720p/50
        {u"HD-SDI-720p/59.94",    IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P59_94)},     // HD-SDI, 720p/59.94
        {u"HD-SDI-720p/60",       IoConfigParams(DTAPI_IOCONFIG_HDSDI, DTAPI_IOCONFIG_720P60)},        // HD-SDI, 720p/60
        {u"phase-noise",          IoConfigParams(DTAPI_IOCONFIG_PHASENOISE)},                          // Phase noise injection
        {u"RS422",                IoConfigParams(DTAPI_IOCONFIG_RS422)},                               // RS422 port
        {u"SDI-receiver",         IoConfigParams(DTAPI_IOCONFIG_SDIRX)},                               // SDI receiver
        {u"SDI-525i/59.94",       IoConfigParams(DTAPI_IOCONFIG_SDI, DTAPI_IOCONFIG_525I59_94)},       // SDI, 525i/59.94
        {u"SDI-625i/50",          IoConfigParams(DTAPI_IOCONFIG_SDI, DTAPI_IOCONFIG_625I50)},          // SDI, 625i/50
        {u"SPI-SDI-525i/59.94",   IoConfigParams(DTAPI_IOCONFIG_SPISDI, DTAPI_IOCONFIG_SPI525I59_94)}, // SD-SDI on a parallel port, 525i/59.94
        {u"SPI-SDI-625i/50",      IoConfigParams(DTAPI_IOCONFIG_SPISDI, DTAPI_IOCONFIG_SPI625I50)},    // SD-SDI on a parallel port, 625i/50
         *
         * End of disabled modes.
         */
    }));
    args.help(u"io-standard",
              u"I/O standard to use on the device port. "
              u"Which modes are supported depends on the device model. "
              u"See the Dektec documentation for more details.");
}


//-----------------------------------------------------------------------------
// Get command line option for Dektec --io-standard option.
//-----------------------------------------------------------------------------

bool ts::GetDektecIOStandardArgs(Args& args, int& value, int& subvalue)
{
    if (args.present(u"io-standard")) {
        const int opt = args.intValue<int>(u"io-standard");
        value = IoConfigValue(opt);
        subvalue = IoConfigSubValue(opt);
        return true;
    }
    else {
        value = subvalue = -1;
        return true;
    }
}


//-----------------------------------------------------------------------------
// Add command line option definitions in an Args for Dektec TS-over-IP options.
//-----------------------------------------------------------------------------

void ts::DefineDektecIPArgs(Args& args, bool receive)
{
    const UString recv_ip(receive ? u"The address part is mandatory for multicast, optional for unicast. " : u"");

    args.option(u"ip4", 0, Args::STRING, 0, 2);
    args.help(u"ip4", u"ipv4-address:port",
              u"TS-over-IP: Destination IPv4 address and port. Either --ip4 or --ip6 must be specified. " +
              recv_ip +
              u"With SMPTE 2022-7 network redundancy, this parameter can be specified twice, main and redundant link.");

    args.option(u"ip6", 0, Args::STRING, 0, 2);
    args.help(u"ip6", u"[ipv6-address]:port",
              u"TS-over-IP: Destination IPv6 address and port. Either --ip4 or --ip6 must be specified. " +
              recv_ip +
              u"The square brackets are literal, as in any IPv6 URL, not an indication of an optional field. "
              u"With SMPTE 2022-7 network redundancy, this parameter can be specified twice, main and redundant link.");

    if (!receive) {
        args.option(u"gw4", 0, Args::STRING, 0, 2);
        args.help(u"gw4", u"ipv4-address",
                  u"TS-over-IP: Specify a non-default IPv4 gateway address. "
                  u"With SMPTE 2022-7 network redundancy, this parameter can be specified twice, main and redundant link.");

        args.option(u"gw6", 0, Args::STRING, 0, 2);
        args.help(u"gw6", u"ipv6-address",
                  u"TS-over-IP: Specify a non-default IPv6 gateway address. "
                  u"With SMPTE 2022-7 network redundancy, this parameter can be specified twice, main and redundant link.");
    }

    if (receive) {
        args.option(u"ssm4-filter", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
        args.help(u"ssm4-filter", u"ipv4-address:port",
                  u"TS-over-IP: Specify IPv4 source-specific multicast (SSM) filter. "
                  u"The port number is optional. "
                  u"This option may be repeated to filter on multiple sources. "
                  u"With SMPTE 2022-7 network redundancy, the same list of filters is used in both links.");

        args.option(u"ssm6-filter", 0, Args::STRING, 0, Args::UNLIMITED_COUNT);
        args.help(u"ssm6-filter", u"[ipv6-address]:port",
                  u"TS-over-IP: Specify IPv6 source-specific multicast (SSM) filter. "
                  u"The port number is optional. "
                  u"The square brackets are literal, as in any IPv6 URL, not an indication of an optional field. "
                  u"This option may be repeated to filter on multiple sources. "
                  u"With SMPTE 2022-7 network redundancy, the same list of filters is used in both links.");
    }
    else {
        args.option(u"source-port", 0, Args::UINT16, 0, 2);
        args.help(u"source-port",
                  u"TS-over-IP: Optional UDP source port for outgoing packets. By default, use a random port. "
                  u"With SMPTE 2022-7 network redundancy, this parameter can be specified twice, main and redundant link.");
    }

    args.option(u"vlan-id", 0, Args::INTEGER, 0, 2, 0, 0x0FFF);
    args.help(u"vlan-id",
              u"TS-over-IP: Optional VLAN identifier as specified in IEEE 802.1Q. "
              u"With SMPTE 2022-7 network redundancy, this parameter can be specified twice, main and redundant link.");

    if (!receive) {
        args.option(u"vlan-priority", 0, Args::INTEGER, 0, 2, 0, 7);
        args.help(u"vlan-priority",
                  u"TS-over-IP: Optional VLAN priority code point as specified in IEEE 802.1Q. "
                  u"With SMPTE 2022-7 network redundancy, this parameter can be specified twice, main and redundant link.");

        args.option(u"ttl", 0, Args::UINT8);
        args.help(u"ttl",
                  u"TS-over-IP: Time-to-live (TTL) value of outgoing IP datagrams.");

        args.option(u"tos", 0, Args::UINT8);
        args.help(u"tos",
                  u"TS-over-IP: Type-of-service (TOS) or differentiated services value of outgoing IP datagrams.");

        args.option(u"ts-per-ip", 0, Args::INTEGER, 0, 1, 1, 7);
        args.help(u"ts-per-ip",
                  u"TS-over-IP: Number of TS packets per IP datagram. The default is 7.");

        args.option(u"rtp");
        args.help(u"rtp",
                  u"TS-over-IP: Use RTP protocol. By default, TS packets are sent in UDP datagrams without header.");
    }

    if (receive) {
        args.option(u"smpte-2022-fec");
        args.help(u"smpte-2022-fec",
                  u"TS-over-IP: Use SMPTE-2022 error correction.");
    }
    else {
        args.option(u"smpte-2022-fec", 0, Names({
            {u"none",    DTAPI_FEC_DISABLE},
            {u"2d-m1",   DTAPI_FEC_2D_M1},
            {u"2d-m1-b", DTAPI_FEC_2D_M1_B},
            {u"2d-m2",   DTAPI_FEC_2D_M2},
            {u"2d-m2-b", DTAPI_FEC_2D_M2_B},
        }));
        args.help(u"smpte-2022-fec",
                  u"TS-over-IP: Specify the SMPTE-2022 error correction mode. The default is none.");

        args.option(u"smpte-2022-d", 0, Args::UNSIGNED);
        args.help(u"smpte-2022-d",
                  u"TS-over-IP with SMPTE-2022 error correction: Specify the number of rows in the FEC matrix, aka 'D' parameter.");

        args.option(u"smpte-2022-l", 0, Args::POSITIVE);
        args.help(u"smpte-2022-l",
                  u"TS-over-IP with SMPTE-2022 error correction: Specify the number of columns in the FEC matrix, aka 'L' parameter.");
    }
}


//-----------------------------------------------------------------------------
// Decode an IP address and/or port and store it into binary data.
//-----------------------------------------------------------------------------

namespace {
    bool DecodeAddress(ts::Args& args, const ts::UChar* option_name, size_t option_index,
                       ts::AbstractNetworkAddress& instance,
                       void* addr, size_t addr_size, ts::AbstractNetworkAddress::Port* port,
                       bool require_addr, bool require_port)
    {
        if (args.count(option_name) <= option_index) {
            // Option not present, not an error, do nothing.
            return true;
        }
        const ts::UString value(args.value(option_name, u"", option_index));
        if (!instance.resolve(value, args)) {
            // Invalid parameter string, error already reported.
            return false;
        }
        if (instance.hasAddress()) {
            instance.getAddress(addr, addr_size);
        }
        else if (require_addr) {
            args.error(u"IP address missing in --%s %s", option_name, value);
            return false;
        }
        if (instance.hasPort() && port != nullptr) {
            *port = instance.port();
        }
        else if (require_port) {
            args.error(u"port number missing in --%s %s", option_name, value);
            return false;
        }
        return true;
    }

    bool DecodeSSM(ts::Args& args, const ts::UChar* option_name,
                   ts::AbstractNetworkAddress& instance,
                   std::vector<Dtapi::DtIpSrcFlt>& filters)
    {
        const size_t count = args.count(option_name);
        for (size_t i = 0; i < count; ++i) {
            Dtapi::DtIpSrcFlt flt;
            TS_ZERO(flt.m_SrcFltIp);
            flt.m_SrcFltPort = 0;
            if (!DecodeAddress(args, option_name, i, instance, flt.m_SrcFltIp, sizeof(flt.m_SrcFltIp), &flt.m_SrcFltPort, true, false)) {
                return false;
            }
            filters.push_back(flt);
        }
        return true;
    }
}


//-----------------------------------------------------------------------------
// Get command line option for Dektec TS-over-IP options.
//-----------------------------------------------------------------------------

bool ts::GetDektecIPArgs(Args& args, bool receive, Dtapi::DtIpPars2& dtpars)
{
    // Clear previous content.
    TS_ZERO(dtpars.m_Ip);
    dtpars.m_Port = 0;
    TS_ZERO(dtpars.m_Gateway);
    dtpars.m_SrcFlt.clear();
    dtpars.m_VlanId = 0;
    dtpars.m_VlanPriority = 0;
    TS_ZERO(dtpars.m_Ip2);
    dtpars.m_Port2 = 0;
    TS_ZERO(dtpars.m_Gateway2);
    dtpars.m_SrcFlt2.clear();
    dtpars.m_VlanId2 = 0;
    dtpars.m_VlanPriority2 = 0;
    dtpars.m_TimeToLive = 0;  // means use default
    dtpars.m_NumTpPerIp = 7;  // default value
    dtpars.m_Protocol = 0;
    dtpars.m_DiffServ = 0;    // means use default
    dtpars.m_FecMode = 0;
    dtpars.m_FecNumRows = 0;
    dtpars.m_FecNumCols = 0;
    dtpars.m_Flags = 0;       // default, implicitly IPv4.
    dtpars.m_Mode = 0;
    dtpars.m_IpProfile.m_Profile = DTAPI_IP_PROF_NOT_DEFINED;
    dtpars.m_IpProfile.m_MaxBitrate = 0;
    dtpars.m_IpProfile.m_MaxSkew = 0;
    dtpars.m_IpProfile.m_VideoStandard = DTAPI_VIDSTD_TS;

    // Use IPv4 or IPv6.
    const bool ipv4 = args.present(u"ip4");
    const bool ipv6 = args.present(u"ip6");
    dtpars.m_Flags = ipv6 ? DTAPI_IP_V6 : DTAPI_IP_V4;

    // Number of links (single or redundant).
    const size_t link_count = std::max(args.count(u"ip4"), args.count(u"ip6"));
    dtpars.m_Mode = link_count <= 1 ? DTAPI_IP_NORMAL : (receive ? DTAPI_IP_RX_2022_7 : DTAPI_IP_TX_2022_7);

    // Check consistency of IPv4 vs. IPv6 and number of links.
    if ((ipv4 && ipv6) ||
        (args.count(u"vlan-id") > link_count) ||
        (receive && !ipv4 && args.present(u"ssm4-filter")) ||
        (receive && !ipv6 && args.present(u"ssm6-filter")) ||
        (!receive && args.count(u"gw4") > args.count(u"ip4")) ||
        (!receive && args.count(u"gw6") > args.count(u"ip6")) ||
        (!receive && args.count(u"source-port") > link_count) ||
        (!receive && args.count(u"vlan-priority") > link_count))
    {
        args.error(u"inconsistent IP parameters, check IPv4 vs. IPv6 and number of links (single vs. redundant)");
        return false;
    }

    // Get IP addresses and ports. Valid for receive and transmit.
    IPAddress ip4, ip6;
    IPSocketAddress sock4, sock6;
    if ((ipv4 && !DecodeAddress(args, u"ip4", 0, sock4, dtpars.m_Ip, sizeof(dtpars.m_Ip), &dtpars.m_Port, !receive, true)) ||
        (ipv4 && !DecodeAddress(args, u"ip4", 1, sock4, dtpars.m_Ip2, sizeof(dtpars.m_Ip2), &dtpars.m_Port2, !receive, true)) ||
        (ipv6 && !DecodeAddress(args, u"ip6", 0, sock6, dtpars.m_Ip, sizeof(dtpars.m_Ip), &dtpars.m_Port, !receive, true)) ||
        (ipv6 && !DecodeAddress(args, u"ip6", 1, sock6, dtpars.m_Ip2, sizeof(dtpars.m_Ip2), &dtpars.m_Port2, !receive, true)) ||
        (!receive && ipv4 && !DecodeAddress(args, u"gw4", 0, ip4, dtpars.m_Gateway, sizeof(dtpars.m_Gateway), nullptr, true, false)) ||
        (!receive && ipv4 && !DecodeAddress(args, u"gw4", 1, ip4, dtpars.m_Gateway2, sizeof(dtpars.m_Gateway2), nullptr, true, false)) ||
        (!receive && ipv6 && !DecodeAddress(args, u"gw6", 0, ip6, dtpars.m_Gateway, sizeof(dtpars.m_Gateway), nullptr, true, false)) ||
        (!receive && ipv6 && !DecodeAddress(args, u"gw6", 1, ip6, dtpars.m_Gateway2, sizeof(dtpars.m_Gateway2), nullptr, true, false)))
    {
        return false;
    }

    // VLAN ids are used in receive and transmit.
    args.getIntValue(dtpars.m_VlanId, u"vlan-id", 0, 0);
    args.getIntValue(dtpars.m_VlanId2, u"vlan-id", 0, 1);

    // Other parameters are interpreted differently from transmit and receive.
    if (receive) {
        // List of SSM filters.
        if ((ipv4 && !DecodeSSM(args, u"ssm4-filter", sock4, dtpars.m_SrcFlt)) ||
            (ipv6 && !DecodeSSM(args, u"ssm6-filter", sock6, dtpars.m_SrcFlt)))
        {
            return false;
        }
        // Same list of SSM filters on both links.
        dtpars.m_SrcFlt2 = dtpars.m_SrcFlt;

        // Other options.
        dtpars.m_Protocol = DTAPI_PROTO_AUTO;
        dtpars.m_FecMode = args.present(u"smpte-2022-fec") ? DTAPI_FEC_2D : DTAPI_FEC_DISABLE;
    }
    else {
        // Transmit. Optional source ports.
        Dtapi::DtIpSrcFlt flt;
        TS_ZERO(flt.m_SrcFltIp);
        args.getIntValue(flt.m_SrcFltPort, u"source-port", 0, 0);
        if (flt.m_SrcFltPort != 0) {
            dtpars.m_SrcFlt.push_back(flt);
        }
        args.getIntValue(flt.m_SrcFltPort, u"source-port", 0, 1);
        if (flt.m_SrcFltPort != 0) {
            dtpars.m_SrcFlt2.push_back(flt);
        }
        if (args.present(u"source-port")) {
            dtpars.m_Flags |= DTAPI_IP_TX_MANSRCPORT;
        }

        // Other options.
        args.getIntValue(dtpars.m_VlanPriority, u"vlan-priority", 0, 0);
        args.getIntValue(dtpars.m_VlanPriority2, u"vlan-priority", 0, 1);
        args.getIntValue(dtpars.m_TimeToLive, u"ttl", 0);
        args.getIntValue(dtpars.m_DiffServ, u"tos", 0);
        args.getIntValue(dtpars.m_NumTpPerIp, u"ts-per-ip", 7); // default: 7
        dtpars.m_Protocol = args.present(u"rtp") ? DTAPI_PROTO_RTP : DTAPI_PROTO_UDP;
        dtpars.m_FecMode = DTAPI_FEC_DISABLE;
        args.getIntValue(dtpars.m_FecMode, u"smpte-2022-fec", DTAPI_FEC_DISABLE);
        args.getIntValue(dtpars.m_FecNumRows, u"smpte-2022-d", 0);
        args.getIntValue(dtpars.m_FecNumCols, u"smpte-2022-l", 0);
    }

    return true;
}


//-----------------------------------------------------------------------------
// Check if Dektec TS-over-IP options are valid.
//-----------------------------------------------------------------------------

bool ts::CheckDektecIPArgs(bool receive, const Dtapi::DtIpPars2& dtpars, Report& report)
{
    // The port is always mandatory. The IP address is optional for receive (unicast).
    if (dtpars.m_Port == 0) {
        report.error(u"missing UDP port number");
        return false;
    }
    if (!receive) {
        const size_t ip_size = (dtpars.m_Flags & DTAPI_IP_V6) != 0 ? 16 : 4;
        bool ok = false;
        for (size_t i = 0; !ok && i < ip_size; ++i) {
            ok = dtpars.m_Ip[i] != 0;
        }
        if (!ok) {
            report.error(u"missing IP address");
            return false;
        }
    }
    return true;
}
