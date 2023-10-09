//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definitions in pcap and pcapng files.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsPlatform.h"

namespace ts {

    //
    // First four bytes of a pcap or pcapng file and byte order magic.
    //
    constexpr uint32_t PCAP_MAGIC_BE   = 0xA1B2C3D4;  //!< Magic number of pcap files, big endian.
    constexpr uint32_t PCAP_MAGIC_LE   = 0xD4C3B2A1;  //!< Magic number of pcap files, little endian.
    constexpr uint32_t PCAPNS_MAGIC_BE = 0xA1B23C4D;  //!< Magic number of pcap files with nanosecond timestamps, big endian.
    constexpr uint32_t PCAPNS_MAGIC_LE = 0x4D3CB2A1;  //!< Magic number of pcap files with nanosecond timestamps, little endian.
    constexpr uint32_t PCAPNG_MAGIC    = 0x0A0D0D0A;  //!< Magic number of pcapng files, not endian dependent.
    constexpr uint32_t PCAPNG_ORDER_BE = 0x1A2B3C4D;  //!< "Byte-order magic" in pcapng files, big endian.
    constexpr uint32_t PCAPNG_ORDER_LE = 0x4D3C2B1A;  //!< "Byte-order magic" in pcapng files, little endian.

    //!
    //! List of pcap-ng block types.
    //!
    enum PcapNgBlockType : uint32_t {
        PCAPNG_SECTION_HEADER  = PCAPNG_MAGIC,  //!< Section header block.
        PCAPNG_INTERFACE_DESC  = 0x00000001,    //!< Interface description block.
        PCAPNG_OBSOLETE_PACKET = 0x00000002,    //!< Obsolete packet block.
        PCAPNG_SIMPLE_PACKET   = 0x00000003,    //!< Enhanced packet block.
        PCAPNG_NAME_RES        = 0x00000004,    //!< Name resolution block.
        PCAPNG_INTERFACE_STAT  = 0x00000005,    //!< Interface statistics block.
        PCAPNG_ENHANCED_PACKET = 0x00000006,    //!< Simple packet block.
        PCAPNG_SYSTEMD_JOURNAL = 0x00000009,    //!< Systemd journal block.
        PCAPNG_DECRYPT_SECRETS = 0x0000000A,    //!< Decryption secrets block.
        PCAPNG_CUSTOM          = 0x00000BAD,    //!< Custom block, can be copied.
        PCAPNG_CUSTOM_NOCOPY   = 0x40000BAD,    //!< Custom block, cannot be copied.
    };

    //!
    //! List of pcap LINKTYPE values.
    //! @see http://www.tcpdump.org/linktypes.html
    //!
    enum PcapLinkType : uint16_t {
        LINKTYPE_NULL                 =   0,  //!< BSD loopback encapsulation.
        LINKTYPE_ETHERNET             =   1,  //!< IEEE 802.3 Ethernet (10Mb, 100Mb, 1000Mb, and up).
        LINKTYPE_AX25                 =   3,  //!< AX.25 packet, with nothing preceding it.
        LINKTYPE_IEEE802_5            =   6,  //!< IEEE 802.5 Token Ring.
        LINKTYPE_ARCNET_BSD           =   7,  //!< ARCNET Data Packets, as described by 878.1-1999.
        LINKTYPE_SLIP                 =   8,  //!< SLIP, encapsulated with a LINKTYPE_SLIP header.
        LINKTYPE_PPP                  =   9,  //!< PPP, as per RFC 1661 and RFC 1662.
        LINKTYPE_FDDI                 =  10,  //!< FDDI, as specified by ANSI INCITS 239-1994.
        LINKTYPE_PPP_HDLC             =  50,  //!< PPP in HDLC-like framing, as per RFC 1662, or Cisco PPP with HDLC framing.
        LINKTYPE_PPP_ETHER            =  51,  //!< PPPoE; the packet begins with a PPPoE header, as per RFC 2516.
        LINKTYPE_ATM_RFC1483          = 100,  //!< RFC 1483 LLC/SNAP-encapsulated ATM.
        LINKTYPE_RAW                  = 101,  //!< Raw IP; the packet begins with an IPv4 or IPv6 header.
        LINKTYPE_C_HDLC               = 104,  //!< Cisco PPP with HDLC framing, as per section 4.3.1 of RFC 1547.
        LINKTYPE_IEEE802_11           = 105,  //!< IEEE 802.11 wireless LAN.
        LINKTYPE_FRELAY               = 107,  //!< Frame Relay LAPF frames.
        LINKTYPE_LOOP                 = 108,  //!< OpenBSD loopback encapsulation.
        LINKTYPE_LINUX_SLL            = 113,  //!< Linux "cooked" capture encapsulation.
        LINKTYPE_LTALK                = 114,  //!< Apple LocalTalk.
        LINKTYPE_PFLOG                = 117,  //!< OpenBSD pflog.
        LINKTYPE_IEEE802_11_PRISM     = 119,  //!< Prism monitor mode information followed by an 802.11 header.
        LINKTYPE_IP_OVER_FC           = 122,  //!< RFC 2625 IP-over-Fibre Channel.
        LINKTYPE_SUNATM               = 123,  //!< ATM traffic, encapsulated as per the scheme used by SunATM devices.
        LINKTYPE_IEEE802_11_RADIOTAP  = 127,  //!< Radiotap link-layer information followed by an 802.11 header.
        LINKTYPE_ARCNET_LINUX         = 129,  //!< ARCNET Data Packets.
        LINKTYPE_APPLE_IP_OVER_IEEE1394 = 138,  //!< Apple IP-over-IEEE 1394 cooked header.
        LINKTYPE_MTP2_WITH_PHDR       = 139,  //!< Signaling System 7 Message Transfer Part Level 2, ITU-T Q.703, preceded by a pseudo-header.
        LINKTYPE_MTP2                 = 140,  //!< Signaling System 7 Message Transfer Part Level 2, ITU-T Q.703.
        LINKTYPE_MTP3                 = 141,  //!< Signaling System 7 Message Transfer Part Level 3, ITU-T Q.704.
        LINKTYPE_SCCP                 = 142,  //!< Signaling System 7 Signalling Connection Control Part, ITU-T Q.711.
        LINKTYPE_DOCSIS               = 143,  //!< DOCSIS MAC frames.
        LINKTYPE_LINUX_IRDA           = 144,  //!< Linux-IrDA packets.
        LINKTYPE_USER0                = 147,  //!< Reserved for private use.
        LINKTYPE_USER15               = 162,  //!< Reserved for private use.
        LINKTYPE_IEEE802_11_AVS       = 163,  //!< AVS monitor mode information followed by an 802.11 header.
        LINKTYPE_BACNET_MS_TP         = 165,  //!< BACnet MS/TP frames.
        LINKTYPE_PPP_PPPD             = 166,  //!< PPP in HDLC-like encapsulation.
        LINKTYPE_GPRS_LLC             = 169,  //!< General Packet Radio Service Logical Link Control.
        LINKTYPE_GPF_T                = 170,  //!< Transparent-mapped generic framing procedure, ITU-T G.7041/Y.1303.
        LINKTYPE_GPF_F                = 171,  //!< Frame-mapped generic framing procedure, ITU-T G.7041/Y.1303.
        LINKTYPE_LINUX_LAPD           = 177,  //!< Link Access Procedures on the D Channel (LAPD) frames.
        LINKTYPE_MFR                  = 182,  //!< FRF.16.1 Multi-Link Frame Relay frames.
        LINKTYPE_BLUETOOTH_HCI_H4     = 187,  //!< Bluetooth HCI UART transport layer.
        LINKTYPE_USB_LINUX            = 189,  //!< USB packets, beginning with a Linux USB header.
        LINKTYPE_PPI                  = 192,  //!< Per-Packet Information information.
        LINKTYPE_IEEE802_15_4_WITHFCS = 195,  //!< IEEE 802.15.4 Low-Rate Wireless Networks.
        LINKTYPE_SITA                 = 196,  //!< Various link-layer types, with a pseudo-header, for SITA.
        LINKTYPE_ERF                  = 197,  //!< Various link-layer types, with a pseudo-header, for Endace DAG cards.
        LINKTYPE_BLUETOOTH_HCI_H4_WITH_PHDR = 201,  //!< Bluetooth HCI UART transport layer.
        LINKTYPE_AX25_KISS            = 202,  //!< AX.25 packet, with a 1-byte KISS header containing a type indicator.
        LINKTYPE_LAPD                 = 203,  //!< Link Access Procedures on the D Channel (LAPD) frames, ITU-T Q.920 and Q.921.
        LINKTYPE_PPP_WITH_DIR         = 204,  //!< PPP, as per RFC 1661 and RFC 1662.
        LINKTYPE_C_HDLC_WITH_DIR      = 205,  //!< Cisco PPP with HDLC framing.
        LINKTYPE_FRELAY_WITH_DIR      = 206,  //!< Frame Relay LAPF frames.
        LINKTYPE_LAPB_WITH_DIR        = 207,  //!< Link Access Procedure, Balanced (LAPB).
        LINKTYPE_IPMB_LINUX           = 209,  //!< IPMB over an I2C circuit.
        LINKTYPE_FLEXRAY              = 210,  //!< FlexRay automotive bus frames.
        LINKTYPE_IEEE802_15_4_NONASK_PHY = 215,  //!< IEEE 802.15.4 Low-Rate Wireless Networks.
        LINKTYPE_USB_LINUX_MMAPPED    = 220,  //!< USB packets, beginning with a Linux USB header.
        LINKTYPE_FC_2                 = 224,  //!< Fibre Channel FC-2 frames, beginning with a Frame_Header.
        LINKTYPE_FC_2_WITH_FRAME_DELIMS = 225,  //!< Fibre Channel FC-2 frames.
        LINKTYPE_IPNET                = 226,  //!< Solaris ipnet pseudo-header.
        LINKTYPE_CAN_SOCKETCAN        = 227,  //!< CAN (Controller Area Network) frames, with a pseudo-header followed by the frame payload.
        LINKTYPE_IPV4                 = 228,  //!< Raw IPv4; the packet begins with an IPv4 header.
        LINKTYPE_IPV6                 = 229,  //!< Raw IPv6; the packet begins with an IPv6 header.
        LINKTYPE_IEEE802_15_4_NOFCS   = 230,  //!< IEEE 802.15.4 Low-Rate Wireless Network.
        LINKTYPE_DBUS                 = 231,  //!< Raw D-Bus messages.
        LINKTYPE_DVB_CI               = 235,  //!< DVB-CI.
        LINKTYPE_MUX27010             = 236,  //!< Variant of 3GPP TS 27.010 multiplexing protocol (similar to, but not the same as, 27.010).
        LINKTYPE_STANAG_5066_D_PDU    = 237,  //!< D_PDUs as described by NATO standard STANAG 5066.
        LINKTYPE_NFLOG                = 239,  //!< Linux netlink NETLINK NFLOG socket log messages.
        LINKTYPE_NETANALYZER          = 240,  //!< Pseudo-header for Hilscher Gesellschaft für Systemautomation mbH netANALYZER devices.
        LINKTYPE_NETANALYZER_TRANSPARENT = 241,  //!< Pseudo-header for Hilscher Gesellschaft für Systemautomation mbH netANALYZER devices.
        LINKTYPE_IPOIB                = 242,  //!< IP-over-InfiniBand.
        LINKTYPE_MPEG_2_TS            = 243,  //!< MPEG-2 Transport Stream transport packets, ISO 13818-1.
        LINKTYPE_NG40                 = 244,  //!< Pseudo-header for ng4T GmbH's UMTS Iub/Iur-over-ATM and Iub/Iur-over-IP format.
        LINKTYPE_NFC_LLCP             = 245,  //!< Pseudo-header for NFC LLCP packet captures.
        LINKTYPE_INFINIBAND           = 247,  //!< Raw InfiniBand frames.
        LINKTYPE_SCTP                 = 248,  //!< SCTP packets, RFC 4960.
        LINKTYPE_USBPCAP              = 249,  //!< USB packets, beginning with a USBPcap header.
        LINKTYPE_RTAC_SERIAL          = 250,  //!< Serial-line packet header for the Schweitzer Engineering Laboratories "RTAC" product.
        LINKTYPE_BLUETOOTH_LE_LL      = 251,  //!< Bluetooth Low Energy air interface Link Layer packets.
        LINKTYPE_NETLINK              = 253,  //!< Linux Netlink capture encapsulation.
        LINKTYPE_BLUETOOTH_LINUX_MONITOR   = 254,  //!< Bluetooth Linux Monitor encapsulation of traffic for the BlueZ stack.
        LINKTYPE_BLUETOOTH_BREDR_BB        = 255,  //!< Bluetooth Basic Rate and Enhanced Data Rate baseband packets.
        LINKTYPE_BLUETOOTH_LE_LL_WITH_PHDR = 256,  //!< Bluetooth Low Energy link-layer packets.
        LINKTYPE_PROFIBUS_DL          = 257,  //!< PROFIBUS data link layer packets.
        LINKTYPE_PKTAP                = 258,  //!< Apple PKTAP capture encapsulation.
        LINKTYPE_EPON                 = 259,  //!< Ethernet-over-passive-optical-network packets.
        LINKTYPE_IPMI_HPM_2           = 260,  //!< IPMI trace packets.
        LINKTYPE_ZWAVE_R1_R2          = 261,  //!< Z-Wave RF profile R1 and R2 packets.
        LINKTYPE_ZWAVE_R3             = 262,  //!< Z-Wave RF profile R3 packets.
        LINKTYPE_WATTSTOPPER_DLM      = 263,  //!< Formats for WattStopper Digital Lighting Management (DLM).
        LINKTYPE_ISO_14443            = 264,  //!< Messages between ISO 14443 contactless smartcards.
        LINKTYPE_RDS                  = 265,  //!< Radio data system (RDS) groups, IEC 62106.
        LINKTYPE_USB_DARWIN           = 266,  //!< USB packets, beginning with a Darwin (macOS, etc.) USB header.
        LINKTYPE_SDLC                 = 268,  //!< SDLC packets.
        LINKTYPE_LORATAP              = 270,  //!< LoRaTap pseudo-header, followed by the payload.
        LINKTYPE_VSOCK                = 271,  //!< Protocol host/guest in VMware and KVM hypervisors.
        LINKTYPE_NORDIC_BLE           = 272,  //!< Messages to and from a Nordic Semiconductor nRF Sniffer for Bluetooth LE packets.
        LINKTYPE_DOCSIS31_XRA31       = 273,  //!< DOCSIS packets and bursts.
        LINKTYPE_ETHERNET_MPACKET     = 274,  //!< mPackets, as specified by IEEE 802.3br Figure 99-4.
        LINKTYPE_DISPLAYPORT_AUX      = 275,  //!< DisplayPort AUX channel monitoring.
        LINKTYPE_LINUX_SLL2           = 276,  //!< Linux "cooked" capture encapsulation v2.
        LINKTYPE_OPENVIZSLA           = 278,  //!< Openvizsla FPGA-based USB sniffer.
        LINKTYPE_EBHSCR               = 279,  //!< Elektrobit High Speed Capture and Replay (EBHSCR) format.
        LINKTYPE_VPP_DISPATCH         = 280,  //!< Records in traces from the http://!<fd.io VPP graph dispatch tracer.
        LINKTYPE_DSA_TAG_BRCM         = 281,  //!< Ethernet frames, with a switch tag.
        LINKTYPE_DSA_TAG_BRCM_PREPEND = 282,  //!< Ethernet frames, with a switch tag.
        LINKTYPE_IEEE802_15_4_TAP     = 283,  //!< IEEE 802.15.4 Low-Rate Wireless Networks.
        LINKTYPE_DSA_TAG_DSA          = 284,  //!< Ethernet frames, with a switch tag.
        LINKTYPE_DSA_TAG_EDSA         = 285,  //!< Ethernet frames, with a programmable Ethernet type switch tag.
        LINKTYPE_ELEE                 = 286,  //!< Payload of lawful intercept packets using the ELEE protocol.
        LINKTYPE_Z_WAVE_SERIAL        = 287,  //!< Serial between host and Z-Wave chip over an RS-232 or USB serial connection.
        LINKTYPE_USB_2_0              = 288,  //!< USB 2.0, 1.1, or 1.0 packet.
        LINKTYPE_ATSC_ALP             = 289,  //!< ATSC Link-Layer Protocol frames.
        LINKTYPE_ETW                  = 290,  //!< Event Tracing for Windows messages.
        LINKTYPE_UNKNOWN              = 0xFFFF  //!< Placeholder for unknown link type.
    };

    //!
    //! List of pcap-ng option codes.
    //!
    enum PcapNgOptionCode : uint32_t {
        PCAPNG_OPT_ENDOFOPT   =  0,  //!< End of option list.
        PCAPNG_OPT_COMMENT    =  1,  //!< Comment.
        PCAPNG_SHB_HARDWARE   =  2,  //!< System hardware.
        PCAPNG_SHB_OS         =  3,  //!< System operating system.
        PCAPNG_SHB_USERAPPL   =  4,  //!< User application.
        PCAPNG_IF_NAME        =  2,  //!< Interface name.
        PCAPNG_IF_DESCRIPTION =  3,  //!< Interface description.
        PCAPNG_IF_IPV4ADDR    =  4,  //!< Interface IPv4 address.
        PCAPNG_IF_IPV6ADDR    =  5,  //!< Interface IPv6 address.
        PCAPNG_IF_MACADDR     =  6,  //!< Interface MAC address.
        PCAPNG_IF_EUIADDR     =  7,  //!< Interface EUI address.
        PCAPNG_IF_SPEED       =  8,  //!< Interface speed in b/s.
        PCAPNG_IF_TSRESOL     =  9,  //!< Time resolution.
        PCAPNG_IF_TZONE       = 10,  //!< Time zone.
        PCAPNG_IF_FILTER      = 11,  //!< Canpture filter.
        PCAPNG_IF_OS          = 12,  //!< Interface operating system.
        PCAPNG_IF_FCSLEN      = 13,  //!< Frame Check Sequence length.
        PCAPNG_IF_TSOFFSET    = 14,  //!< Timestamps offset.
        PCAPNG_IF_HARDWARE    = 15,  //!< Interface hardware.
        PCAPNG_IF_TXSPEED     = 16,  //!< Interface transmission speed.
        PCAPNG_IF_RXSPEED     = 17,  //!< Interface reception speed.
    };
}
