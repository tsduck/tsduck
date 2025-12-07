# Advanced forms of television over IP multicast

This directory contains code to support some advanced forms of television over IP multicast,
not using MPEG-2 TS as base transport format. Instead, they start from formats coming the
streaming industry such as DASH or HLS and encapsulate them in UDP-IP packets.

Therefore, the usual "IP-TV" which consists in multicasting MPEG-2 TS packets over UDP is
not part of these standards because it is essentially classical digital television over
IP multicast.

Some of the known high-level standards are:

- DVB-NIP, Native IP content delivery.
- DVB-I, discovery of television services over IP.
- DVB-MABR, adaptive bitrate over multicast.
- ATSC 3.0, the next generation of American terrestrial TV over broadcast IP.
- ABNT TV 3.0, a Brazilian standard, based on ATSC 3.0.

## Protocols stack

The basic common principle is the delivery of "files" over multicast UDP using one of the
FLUTE or ROUTE delivery protocols. These two protocols are based on the following stack:

1. GSE or MPE (see below)
2. IP/UDP
3. LCT: Layered Coding Transport
4. ALC: Asynchronous Layered Coding
5. FEC: Forward Error Correction
6. FLUTE or ROUTE
7. DVB-NIP, DVB-MABR, ATSC 3.0

## Relevant standards

- IETF RFC 3048: "Reliable Multicast Transport Building Blocks for One-to-Many Bulk-Data Transfer", January 2001
- IETF RFC 3695: "Compact Forward Error Correction (FEC) Schemes", February 2004
- IETF RFC 3926: "FLUTE - File Delivery over Unidirectional Transport", October 2004 (FLUTE v1)
- IETF RFC 5052: "Forward Error Correction (FEC) Building Block", August 2007
- IETF RFC 5651: "Layered Coding Transport (LCT) Building Block", October 2009
- IETF RFC 5775: "Asynchronous Layered Coding (ALC) Protocol Instantiation", April 2010
- IETF RFC 6726: "FLUTE - File Delivery over Unidirectional Transport", October 2004 (FLUTE v2)
- IETF RFC 9223: "Real-Time Transport Object Delivery over Unidirectional Transport (ROUTE)", April 2022
- ETSI TS 103 769: "Digital Video Broadcasting (DVB); Adaptive media streaming over IP multicast", V1.2.1, November 2024
- ETSI TS 103 876: "Digital Video Broadcasting (DVB); Native IP Broadcasting", V1.1.1, September 2024
- ETSI TS 103 770: "Digital Video Broadcasting (DVB); Service Discovery and Programme Metadata for DVB-I", V1.2.1, September 2024

Note: FLUTE v2 is not backward compatible with FLUTE v1. DVB-NIP uses FLUTE v1.

## Acronyms

- ALC: Asynchronous Layered Coding
- FDT: File Delivery Table
- FEC: Forward Error Correction
- LCT: Layered Coding Transport
- NIF: Network Information File
- SIF: Service Information File
- SLEP: Service List Entry Point
- TOI: Transport Object Identifier
- TSI: Transport Session Identifier

## Limitations

The current implementation of DVB-NIP in TSDuck has the following limitations:

- FLUTE only. DVB-NIP over ROUTE is not supported.
- The only supported FEC Encoding ID is 0, 'Compact No-Code FEC' (Fully-Specified).

## DVB-NIP

DVB-NIP means "Native IP". It specifies the signalization and transport of DTV
programs over native IP networks. DVB-NIP is transported into UDP datagrams,
using IPv4 or IPv6.

UDP datagrams with DVB-NIP content can be carried over GSE (Generic Stream Encapsulation)
or MPE (Multi-Protocol Encapsulation). GSE can be natively transmitted over DVB-S2,
DVB-S2X or DVB-T2. In that case, the GSE frames contain UDP datagrams instead of TS packets.
MPE, on the other hand, is transmitted inside TS packets (inside DSM-CC sections which are
transmitted in a PID of the TS).

TSDuck is a transport stream framework. As such, it can only analyze DVB-NIP over MPE.
GSE frames are currently inaccessible to consumer tuners.

A DVB-NIP stream is encapsulated into one single MPE stream. This MPE stream must be
properly declared into the PMT of a service. This service must have one single MPE stream.
A TS may carry several DVB-NIP streams but they must be in distinct services, one per
DVB-NIP stream.

## FEC, Forward Error Correction

- FEC Encoding ID: An integer which defines the type of FEC. With FLUTE, it is stored in
  the codepoint field of the LCT header (RFC 3926, section 5.1).

- FEC Object Transmission Information: A structure which is stored int the `EXT_FTI` extension
  of the LCT heaer

- FEC Payload ID: A structure which is stored immediately after the LCT header (and its
  extensions), immediately before the packet payload. The format of the FEC Payload ID
  structure depends on the FEC Encoding ID.

## File transport protocol

DVB-NIP is based on FLUTE or ROUTE, two similar protocols to broadcast files.
FLUTE and ROUTE are identically based on ALC, which is itself based on LCT.
Therefore, a UDP datagram carrying DVB-NIP content starts with a LCT header.

Open question: DVB-NIP specifies that it can be transported over FLUTE or ROUTE.
However, the standard does not specifies how to declare the transport protocol.
Because FLUTE and ROUTE are both based on LCT/ALC, the UDP packets are very similar,
starting with a LCT header in both cases. Because LCT does not define which protocol
is carried, it is impossible to distinguish a FLUTE packet and a ROUTE packet.
So, how does a receiver decide whether a DVB-NIP stream is based on FLUTE or ROUTE?
Nobody knows...

The current implementation of DVB-NIP in TSDuck assumes that FLUTE is used, not ROUTE.

## DVB-NIP definitions

- Announcement channel: multicast destination 224.0.23.14:3937.
- Bootstrap stream: the DVB-NIP standard is unclear.
  - Can be the same as announcement.
  - Can be several streams, typically one per operator, which are listed in the
    "bootstrap" file in the anouncement channel.
- NIP stream identifier: NIPNetworkID, NIPCarrierID, NIPLinkID, NIPServiceID.

## Service discovery

- Listen on announcement channel (destination 224.0.23.14:3937).
  - Get bootstrap multicast gateway configuration instance document.
    - Name: `urn:dvb:metadata:cs:NativeIPMulticastTransportObjectTypeCS:2023:bootstrap`
    - Type: `application/xml+dvb-mabr-session-configuration`
  - Get NIP Network Information File (NIF).
    - Name: `urn:dvb:metadata:nativeip:NetworkInformationFile`
    - Type: `application/xml+dvb-nip-nif`
  - Get NIP Service Information File (SIF).
    - Name: `urn:dvb:metadata:nativeip:ServiceInformationFile`
    - Type: `application/xml+dvb-nip-sif`

- The NIF contains a list of physical networks (e.g. satellite positions).
  - A physical network contains several DVB-NIP streams.
  - A DVB-NIP stream is defined by:
    - Its logical NIP stream identifier.
    - Its physical parameters (modulation parameters for the physical stream, TS or GSE).

- The SIF contains a list of "broadcast media streams".
  - A broadcast media stream contains:
    - Its logical NIP stream identifier.
    - A list of service URI's and a list of interactive applications.
    - Each service URI may point to:
      - A service list file (`.xml`).
      - A HLS play list (`.m3u8`) for a service.
      - A MPEG-DASH manifest (`.mpd`) for a service.
    
- The "bootstrap" file is a MulticastGatewayConfiguration.
  - It contains several "transport sessions".
  - A transport session contains:
    - An end point: source IP address, destination IP address and UDP port, TSI.
    - The transport protocol: FLUTE or ROUTE.
    - Bitrate information.
    - An optional list of important files in that session (tables, service lists, manifests or playlists, images, etc).

## FLUTE definitions

- Channel = source IP address / destination IP address and UDP port
- Session = source IP address / TSI
- Object = session / TOI = source IP address / TSI / TOI

- ALC/LCT session = FLUTE file delivery session
- ALC/LCT object = FILE file or FDT Instance

The source IP address that carries the signalling information shall be unique for each NIP Stream.

Session:

- TOI ’0’ is reserved for delivery of FDT Instances.
- Each FDT Instance is uniquely identified by an FDT Instance ID.
- FDT Instance is identified in LCT Header Extension EXT_FDT.
- FDT provide a file description entry mapped to a TOI for each file appearing within the session.
- FDT maps a TOI to a URL.
- A TOI can be described in multiple FDT instances.

Attributes related to the delivery of file:

- TOI value that represents the file
- FEC Object Transmission Information (including the FEC Encoding ID and, if relevant, the FEC Instance ID)
- Size of the transport object carrying the file
- Aggregate rate of sending packets to all channels

Attributes related to the file itself:

- Name, Identification and Location of file (specified by the URI)
- MIME media type of file
- Size of file
- Encoding of file
- Message digest of file

The standard says that "it is recommended that FDT Instance that contains the file
description entry for a file is sent prior to the sending of the described file
within a file delivery session". This means that it could be after.
