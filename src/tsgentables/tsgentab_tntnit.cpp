//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
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
//  tsgentab plugin shared library: TNT France NIT
//
//----------------------------------------------------------------------------

#include "tsGenTabPlugin.h"
#include "tsTables.h"

// 0xE9 = 233 = \351 = e acute
// 0xE7 = 231 = \347 = c cedilla
#define TNT_OLD_NAME_LATIN1 "r\351seau num\351rique terrestre fran\347ais"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class TNTNITPlugin: public GenTabPlugin
    {
    public:
        TNTNITPlugin();
        virtual void generate(AbstractTablePtr&);
    private:
        bool add_service_list;
        bool split_lcn;
        void generateV23(AbstractTablePtr&);
        void generateV24(AbstractTablePtr&);
        void generateV25(AbstractTablePtr&);
        void generateV26(AbstractTablePtr&);
    };
}

TSGENTAB_DECLARE_PLUGIN(ts::TNTNITPlugin)


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::TNTNITPlugin::TNTNITPlugin() :
    GenTabPlugin("TNT France NIT", "[options]"),
    add_service_list(false),
    split_lcn(false)
{
    option("nit-version",     'v', INTEGER, 0, 1, 23, 26);
    option("no-service-list", 'n');
    option("split-lcn",        0);

    setHelp("Options:\n"
            "\n"
            "  --help\n"
            "      Display this help text.\n"
            "\n"
            "  -v value\n"
            "  -nit-version value\n"
            "      Specifies the table version of the NIT. The supported versions are:\n"
            "      - 26 (jan. 2010), the default\n"
            "      - 25 (30 oct. 2008)\n"
            "      - 24 (oct. 2007)\n"
            "      - 23 (sep. 2007)\n"
            "\n"
            "  -n\n"
            "  --no-service-list\n"
            "      Omit the service_list_descriptor in each transport stream.\n"
            "\n"
            "  --split-lcn\n"
            "      Split some logical_channel_number_descriptors and\n"
            "      HD_simulcast_logical_channel_descriptors in two parts.\n"
            "      This option is available for NIT versions 23 and 24 only.\n"
            "\n"
            "  --version\n"
            "      Display the version number.\n");
}


//----------------------------------------------------------------------------
// Table Generation
//----------------------------------------------------------------------------

void ts::TNTNITPlugin::generate(AbstractTablePtr& table)
{
    add_service_list = !present("no-service-list");
    split_lcn = present("split-lcn");

    switch (intValue("nit-version", 26)) {
        case 23: generateV23(table); break;
        case 24: generateV24(table); break;
        case 25: generateV25(table); break;
        case 26: generateV26(table); break;
        default: assert(false);
    }
}


//----------------------------------------------------------------------------
// Table Generation - NIT version 26 - jan 2010
//----------------------------------------------------------------------------

void ts::TNTNITPlugin::generateV26(AbstractTablePtr& table)
{
    // Create table

    NIT* nit = new NIT();
    table = nit;

    // Transport stream id

    TransportStreamId r1(1, NID_TNT_FRANCE);
    TransportStreamId r2(2, NID_TNT_FRANCE);
    TransportStreamId r3(3, NID_TNT_FRANCE);
    TransportStreamId r4(4, NID_TNT_FRANCE);
    TransportStreamId r5(5, NID_TNT_FRANCE);
    TransportStreamId r6(6, NID_TNT_FRANCE);
    TransportStreamId l8(8, NID_TNT_FRANCE);

    // The TNT NIT v26 introduces hand-crafted segmentation.
    // Each TS is placed in a specific section.

    nit->section_hints[r1] = 0;
    nit->section_hints[r2] = 0;
    nit->section_hints[r3] = 0;
    nit->section_hints[r4] = 0;
    nit->section_hints[r5] = 0;
    nit->section_hints[r6] = 0;
    nit->section_hints[l8] = 0;

    // Fixed fields

    nit->version = 26;
    nit->is_current = true;
    nit->network_id = NID_TNT_FRANCE;

    // All terrestrial delivery descriptors are generic

    TerrestrialDeliverySystemDescriptor terrestrial_delivery;
    terrestrial_delivery.centre_frequency = 0xFFFFFFFF;
    terrestrial_delivery.bandwidth = 0;          // 8 Mhz
    terrestrial_delivery.high_priority = true;
    terrestrial_delivery.no_time_slicing = true;
    terrestrial_delivery.no_mpe_fec = true;
    terrestrial_delivery.constellation = 2;      // 64-QAM
    terrestrial_delivery.hierarchy = 0;          // non-hierarchical, native interleaver
    terrestrial_delivery.code_rate_hp = 7;       // reserved
    terrestrial_delivery.code_rate_lp = 0;       // 1/2
    terrestrial_delivery.guard_interval = 0;     // 1/32
    terrestrial_delivery.transmission_mode = 1;  // 8K
    terrestrial_delivery.other_frequency = false;

    // Main descriptor loop

    nit->descs.add(NetworkNameDescriptor("F"));
    nit->descs.add(SSULinkageDescriptor(1, nit->network_id, 0x01FF, OUI_DVB));
    nit->descs.add(SSULinkageDescriptor(2, nit->network_id, 0x02FF, OUI_DVB));
    nit->descs.add(SSULinkageDescriptor(3, nit->network_id, 0x03FF, OUI_DVB));
    nit->descs.add(SSULinkageDescriptor(4, nit->network_id, 0x04FF, OUI_DVB));
    nit->descs.add(SSULinkageDescriptor(5, nit->network_id, 0x05FF, OUI_DVB));
    nit->descs.add(SSULinkageDescriptor(6, nit->network_id, 0x06FF, OUI_DVB));

    // R1

    nit->transports[r1].add(PrivateDataSpecifierDescriptor(PDS_EICTA));
    nit->transports[r1].add(LogicalChannelNumberDescriptor(
        0x0101,  2,    // France 2
        0x0104,  5,    // France 5
        0x0105, 19,    // France O national
        0x0106, 13,    // LCP
        0x0111,  3,    // France 3a
        0x0112,  3,    // France 3b
        0x0113,  3,    // France 3c
        0x0114,  3,    // France 3
        0x0115,  3,    // France 3
        0x0116,  3,    // France 3
        0x0117,  3,    // France 3
        0x0118,  3,    // France 3 Poitiers
        0x0119,  3,    // France 3
        0x011A,  3,    // France 3 Toulouse
        0x011B,  3,    // France 3 Rhone
        0x011C,  3,    // France 3
        0x011D,  3,    // France 3
        0x011E,  3,    // France 3 Berry
        0x011F,  3,    // France 3
        0x0120,  3,    // France 3
        0x0121,  3,    // France 3
        0x0122,  3,    // France 3 Le Havre
        0x0123,  3,    // France 3
        0x0124,  3,    // France 3
        0x0125,  3,    // France 3 Orleans
        0x0126,  3,    // France 3
        0x0127,  3,    // France 3
        0x0128,  3,    // France 3
        0x0129,  3,    // France 3
        0x012A,  3,    // France 3
        0x012B,  3,    // France 3
        0x012C,  3,    // France 3
        0x012D,  3,    // France 3
        0x012E,  3,    // France 3
        0x012F,  3,    // France 3
        0x0130,  3,    // France 3
        0x0131,  3,    // France 3
        0x0132,  3,    // France 3 Dijon
        0x0133,  3,    // France 3 Montpellier
        0x0134,  3,    // France 3
        0x0135,  3,    // France 3
        0x0136,  3,    // France 3
        0x0137,  3,    // France 3
        0x0138,  3,    // France 3 Provence
        0x0139,  3,    // France 3
        0x013A,  3,    // France 3 Champardenne
        0x013B,  3,    // France 3
        0x0143, 22,    // France 3 LCN 22 a
        0x0144, 22,    // France 3 LCN 22 b
        0x0170, 20,    // Locale LCN 20
        0x0171, 21,    // Locale LCN 21
        0x0172, 22,    // Locale LCN 22
        0x0173, 23,    // Locale LCN 23
        0x0174, 24,    // Locale LCN 24
        0x0175, 25,    // Locale LCN 25
        0x0176, 20,    // France O regional IDF
        -1));
    nit->transports[r1].add(HDSimulcastLogicalChannelDescriptor(
        0x0101, 52,  // France 2 -> France 2 HD
        -1));
    if (add_service_list) {
        nit->transports[r1].add(ServiceListDescriptor(
            0x0101, 0x01,  // France 2
            0x0104, 0x01,  // France 5
            0x0105, 0x01,  // France O
            0x0106, 0x01,  // LCP
            0x0111, 0x01,  // France 3a
            0x0112, 0x01,  // France 3b
            0x0113, 0x01,  // France 3c
            0x0114, 0x01,  // France 3
            0x0115, 0x01,  // France 3
            0x0116, 0x01,  // France 3
            0x0117, 0x01,  // France 3
            0x0118, 0x01,  // France 3 Poitiers
            0x0119, 0x01,  // France 3
            0x011A, 0x01,  // France 3 Toulouse
            0x011B, 0x01,  // France 3 Rhone
            0x011C, 0x01,  // France 3
            0x011D, 0x01,  // France 3
            0x011E, 0x01,  // France 3 Berry
            0x011F, 0x01,  // France 3
            0x0120, 0x01,  // France 3
            0x0121, 0x01,  // France 3
            0x0122, 0x01,  // France 3 Le Havre
            0x0123, 0x01,  // France 3
            0x0124, 0x01,  // France 3
            0x0125, 0x01,  // France 3 Orleans
            0x0126, 0x01,  // France 3
            0x0127, 0x01,  // France 3
            0x0128, 0x01,  // France 3
            0x0129, 0x01,  // France 3
            0x012A, 0x01,  // France 3
            0x012B, 0x01,  // France 3
            0x012C, 0x01,  // France 3
            0x012D, 0x01,  // France 3
            0x012E, 0x01,  // France 3
            0x012F, 0x01,  // France 3
            0x0130, 0x01,  // France 3
            0x0131, 0x01,  // France 3
            0x0132, 0x01,  // France 3 Dijon
            0x0133, 0x01,  // France 3 Montpellier
            0x0134, 0x01,  // France 3
            0x0135, 0x01,  // France 3
            0x0136, 0x01,  // France 3
            0x0137, 0x01,  // France 3
            0x0138, 0x01,  // France 3 Provence
            0x0139, 0x01,  // France 3
            0x013A, 0x01,  // France 3 Champardenne
            0x013B, 0x01,  // France 3
            0x0143, 0x01,  // France 3 LCN 22 a
            0x0144, 0x01,  // France 3 LCN 22 b
            0x0170, 0x01,  // Locale LCN 20
            0x0171, 0x01,  // Locale LCN 21
            0x0172, 0x01,  // Locale LCN 22
            0x0173, 0x01,  // Locale LCN 23
            0x0174, 0x01,  // Locale LCN 24
            0x0175, 0x01,  // Locale LCN 25
            0x0176, 0x01,  // France O
            -1));
    }
    nit->transports[r1].add(terrestrial_delivery);

    // R2

    nit->transports[r2].add(PrivateDataSpecifierDescriptor(PDS_EICTA));
    nit->transports[r2].add(LogicalChannelNumberDescriptor(
        0x0201,  8,    // Direct 8
        0x0203, 15,    // BFM TV
        0x0204, 16,    // i> Tele
        0x0205, 17,    // Europe 2 TV / Virgin 17
        0x0206, 18,    // Gulli
        0x0207, 14,    // France 4
        -1));
    if (add_service_list) {
        nit->transports[r2].add(ServiceListDescriptor(
            0x0201, 0x01,  // Direct 8
            0x0203, 0x01,  // BFM TV
            0x0204, 0x01,  // i> Tele
            0x0205, 0x01,  // Europe 2 TV / Virgin 17
            0x0206, 0x01,  // Gulli
            0x0207, 0x01,  // France 4
            -1));
    }
    nit->transports[r2].add(terrestrial_delivery);

    // R3

    nit->transports[r3].add(PrivateDataSpecifierDescriptor(PDS_EICTA));
    nit->transports[r3].add(LogicalChannelNumberDescriptor(
        0x0301,  4,    // Canal+
        0x0302, 33,    // Canal+ Cinema
        0x0303, 32,    // Canal+ Sport
        0x0304, 35,    // Planete
        0x0305, 37,    // Canal J
        0x0306, 30,    // TPS Star
        -1));
    if (add_service_list) {
        nit->transports[r3].add (ServiceListDescriptor (
            0x0301, 0x01,  // Canal+
            0x0302, 0x01,  // Canal+ Cinema
            0x0303, 0x01,  // Canal+ Sport
            0x0304, 0x01,  // Planete
            0x0305, 0x01,  // Canal J
            0x0306, 0x01,  // TPS Star
            -1));
    }
    nit->transports[r3].add (terrestrial_delivery);

    // R4

    nit->transports[r4].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r4].add (LogicalChannelNumberDescriptor (
        0x0401,  6,    // M6
        0x0402,  9,    // W9
        0x0403, 11,    // NT1
        0x0404, 31,    // Paris Premiere
        0x0407, 57,    // Arte HD
        -1));
    nit->transports[r4].add (HDSimulcastLogicalChannelDescriptor (
        0x0401, 56,  // M6 -> M6 HD
        0x0407,  7,  // Arte HD -> Arte
        -1));
    if (add_service_list) {
        nit->transports[r4].add (ServiceListDescriptor (
            0x0401, 0x01,  // M6
            0x0402, 0x01,  // W9
            0x0403, 0x01,  // NT1
            0x0404, 0x01,  // Paris Premiere
            0x0407, 0x01,  // Arte HD
            -1));
    }
    nit->transports[r4].add (terrestrial_delivery);

    // R5

    nit->transports[r5].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r5].add (LogicalChannelNumberDescriptor (
        0x0501, 51,    // TF1 HD
        0x0502, 52,    // France 2 HD
        0x0503, 56,    // M6 HD
        -1));
    nit->transports[r5].add (HDSimulcastLogicalChannelDescriptor (
        0x0501, 1,  // TF1 HD -> TF1
        0x0502, 2,  // France 2 HD -> France 2
        0x0503, 6,  // M6 HD -> M6
        -1));
    if (add_service_list) {
        nit->transports[r5].add (ServiceListDescriptor (
            0x0501, 0x01,  // TF1 HD
            0x0502, 0x01,  // France 2 HD
            0x0503, 0x01,  // M6 HD
            -1));
    }
    nit->transports[r5].add (terrestrial_delivery);

    // R6

    nit->transports[r6].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r6].add (LogicalChannelNumberDescriptor (
        0x0601,  1,    // TF1
        0x0602, 12,    // NRJ12
        0x0603, 38,    // LCI
        0x0604, 39,    // Eurosport France
        0x0605, 36,    // TF6
        0x0606, 10,    // TMC
        0x0607,  7,    // Arte
        -1));
    nit->transports[r6].add (HDSimulcastLogicalChannelDescriptor (
        0x0601, 51,  // TF1 -> TF1 HD
        0x0607, 57,  // Arte -> Arte HD
        -1));
    if (add_service_list) {
        nit->transports[r6].add (ServiceListDescriptor (
            0x0601, 0x01,  // TF1
            0x0602, 0x01,  // NRJ12
            0x0603, 0x01,  // LCI
            0x0604, 0x01,  // Eurosport France
            0x0605, 0x01,  // TF6
            0x0606, 0x01,  // TMC
            0x0607, 0x01,  // Arte
            -1));
    }
    nit->transports[r6].add (terrestrial_delivery);

    // L8

    nit->transports[l8].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[l8].add (LogicalChannelNumberDescriptor (
        0x0801, 20,    // Locale LCN 20
        0x0802, 21,    // Locale LCN 21
        0x0803, 22,    // Locale LCN 22
        0x0804, 23,    // Locale LCN 23
        0x0805, 24,    // Locale LCN 24
        0x0806, 25,    // Locale LCN 25
        0x0883, 23,    // France 3 LCN 23
        -1));
    if (add_service_list) {
        nit->transports[l8].add (ServiceListDescriptor (
            0x0801, 0x01,  // Locale LCN 20
            0x0802, 0x01,  // Locale LCN 21
            0x0803, 0x01,  // Locale LCN 22
            0x0804, 0x01,  // Locale LCN 23
            0x0805, 0x01,  // Locale LCN 24
            0x0806, 0x01,  // Locale LCN 25
            0x0883, 0x01,  // France 3 LCN 23
            -1));
    }
    nit->transports[l8].add (terrestrial_delivery);
}


//----------------------------------------------------------------------------
// Table Generation - NIT version 25 - 30 oct 2008
//----------------------------------------------------------------------------

void ts::TNTNITPlugin::generateV25 (AbstractTablePtr& table)
{
    // Create table

    NIT* nit = new NIT();
    table = nit;

    // Transport stream id

    TransportStreamId r1 (1, NID_TNT_FRANCE);
    TransportStreamId r2 (2, NID_TNT_FRANCE);
    TransportStreamId r3 (3, NID_TNT_FRANCE);
    TransportStreamId r4 (4, NID_TNT_FRANCE);
    TransportStreamId r5 (5, NID_TNT_FRANCE);
    TransportStreamId r6 (6, NID_TNT_FRANCE);
    TransportStreamId l8 (8, NID_TNT_FRANCE);

    // Fixed fields

    nit->version = 25;
    nit->is_current = true;
    nit->network_id = NID_TNT_FRANCE;

    // All terrestrial delivery descriptors are generic

    TerrestrialDeliverySystemDescriptor terrestrial_delivery;
    terrestrial_delivery.centre_frequency = 0xFFFFFFFF;
    terrestrial_delivery.bandwidth = 0;          // 8 Mhz
    terrestrial_delivery.high_priority = true;
    terrestrial_delivery.no_time_slicing = true;
    terrestrial_delivery.no_mpe_fec = true;
    terrestrial_delivery.constellation = 2;      // 64-QAM
    terrestrial_delivery.hierarchy = 0;          // non-hierarchical, native interleaver
    terrestrial_delivery.code_rate_hp = 7;       // reserved
    terrestrial_delivery.code_rate_lp = 0;       // 1/2
    terrestrial_delivery.guard_interval = 0;     // 1/32
    terrestrial_delivery.transmission_mode = 1;  // 8K
    terrestrial_delivery.other_frequency = false;

    // Main descriptor loop

    nit->descs.add (NetworkNameDescriptor ("F"));
    nit->descs.add (SSULinkageDescriptor (1, nit->network_id, 0x01FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (2, nit->network_id, 0x02FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (3, nit->network_id, 0x03FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (4, nit->network_id, 0x04FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (5, nit->network_id, 0x05FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (6, nit->network_id, 0x06FF, OUI_DVB));

    // R1

    nit->transports[r1].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r1].add (LogicalChannelNumberDescriptor (
        0x0101,  2,    // France 2
        0x0104,  5,    // France 5
        0x0105,  7,    // Arte
        0x0106, 13,    // LCP
        0x0111,  3,    // France 3a
        0x0112,  3,    // France 3b
        0x0113,  3,    // France 3c
        0x0114,  3,    // France 3
        0x0115,  3,    // France 3
        0x0116,  3,    // France 3
        0x0117,  3,    // France 3
        0x0118,  3,    // France 3 Poitiers
        0x0119,  3,    // France 3
        0x011A,  3,    // France 3 Toulouse
        0x011B,  3,    // France 3 Rhone
        0x011C,  3,    // France 3
        0x011D,  3,    // France 3
        0x011E,  3,    // France 3 Berry
        0x011F,  3,    // France 3
        0x0120,  3,    // France 3
        0x0121,  3,    // France 3
        0x0122,  3,    // France 3 Le Havre
        0x0123,  3,    // France 3
        0x0124,  3,    // France 3
        0x0125,  3,    // France 3 Orleans
        0x0126,  3,    // France 3
        0x0127,  3,    // France 3
        0x0128,  3,    // France 3
        0x0129,  3,    // France 3
        0x012A,  3,    // France 3
        0x012B,  3,    // France 3
        0x012C,  3,    // France 3
        0x012D,  3,    // France 3
        0x012E,  3,    // France 3
        0x012F,  3,    // France 3
        0x0130,  3,    // France 3
        0x0131,  3,    // France 3
        0x0132,  3,    // France 3 Dijon
        0x0133,  3,    // France 3 Montpellier
        0x0134,  3,    // France 3
        0x0135,  3,    // France 3
        0x0136,  3,    // France 3
        0x0137,  3,    // France 3
        0x0138,  3,    // France 3 Provence
        0x0139,  3,    // France 3
        0x013A,  3,    // France 3 Champardenne
        0x013B,  3,    // France 3
        0x0143, 22,    // France 3 LCN 22 a
        0x0144, 22,    // France 3 LCN 22 b
        0x0170, 20,    // Locale LCN 20
        0x0171, 21,    // Locale LCN 21
        0x0172, 22,    // Locale LCN 22
        0x0173, 23,    // Locale LCN 23
        0x0174, 24,    // Locale LCN 24
        0x0175, 25,    // Locale LCN 25
        0x0176, 20,    // France O
        -1));
    nit->transports[r1].add (HDSimulcastLogicalChannelDescriptor (
        0x0101, 52,  // France 2 -> France 2 HD
        0x0105, 57,  // Arte -> Arte HD
        -1));
    if (add_service_list) {
        nit->transports[r1].add (ServiceListDescriptor (
            0x0101, 0x01,  // France 2
            0x0104, 0x01,  // France 5
            0x0105, 0x01,  // Arte
            0x0106, 0x01,  // LCP
            0x0111, 0x01,  // France 3a
            0x0112, 0x01,  // France 3b
            0x0113, 0x01,  // France 3c
            0x0114, 0x01,  // France 3
            0x0115, 0x01,  // France 3
            0x0116, 0x01,  // France 3
            0x0117, 0x01,  // France 3
            0x0118, 0x01,  // France 3 Poitiers
            0x0119, 0x01,  // France 3
            0x011A, 0x01,  // France 3 Toulouse
            0x011B, 0x01,  // France 3 Rhone
            0x011C, 0x01,  // France 3
            0x011D, 0x01,  // France 3
            0x011E, 0x01,  // France 3 Berry
            0x011F, 0x01,  // France 3
            0x0120, 0x01,  // France 3
            0x0121, 0x01,  // France 3
            0x0122, 0x01,  // France 3 Le Havre
            0x0123, 0x01,  // France 3
            0x0124, 0x01,  // France 3
            0x0125, 0x01,  // France 3 Orleans
            0x0126, 0x01,  // France 3
            0x0127, 0x01,  // France 3
            0x0128, 0x01,  // France 3
            0x0129, 0x01,  // France 3
            0x012A, 0x01,  // France 3
            0x012B, 0x01,  // France 3
            0x012C, 0x01,  // France 3
            0x012D, 0x01,  // France 3
            0x012E, 0x01,  // France 3
            0x012F, 0x01,  // France 3
            0x0130, 0x01,  // France 3
            0x0131, 0x01,  // France 3
            0x0132, 0x01,  // France 3 Dijon
            0x0133, 0x01,  // France 3 Montpellier
            0x0134, 0x01,  // France 3
            0x0135, 0x01,  // France 3
            0x0136, 0x01,  // France 3
            0x0137, 0x01,  // France 3
            0x0138, 0x01,  // France 3 Provence
            0x0139, 0x01,  // France 3
            0x013A, 0x01,  // France 3 Champardenne
            0x013B, 0x01,  // France 3
            0x0143, 0x01,  // France 3 LCN 22 a
            0x0144, 0x01,  // France 3 LCN 22 b
            0x0170, 0x01,  // Locale LCN 20
            0x0171, 0x01,  // Locale LCN 21
            0x0172, 0x01,  // Locale LCN 22
            0x0173, 0x01,  // Locale LCN 23
            0x0174, 0x01,  // Locale LCN 24
            0x0175, 0x01,  // Locale LCN 25
            0x0176, 0x01,  // France O
            -1));
    }
    nit->transports[r1].add (terrestrial_delivery);

    // R2

    nit->transports[r2].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r2].add (LogicalChannelNumberDescriptor (
        0x0201,  8,    // Direct 8
        0x0203, 15,    // BFM TV
        0x0204, 16,    // i> Tele
        0x0205, 17,    // Europe 2 TV / Virgin 17
        0x0206, 18,    // Gulli
        0x0207, 14,    // France 4
        -1));
    if (add_service_list) {
        nit->transports[r2].add (ServiceListDescriptor (
            0x0201, 0x01,  // Direct 8
            0x0203, 0x01,  // BFM TV
            0x0204, 0x01,  // i> Tele
            0x0205, 0x01,  // Europe 2 TV / Virgin 17
            0x0206, 0x01,  // Gulli
            0x0207, 0x01,  // France 4
            -1));
    }
    nit->transports[r2].add (terrestrial_delivery);

    // R3

    nit->transports[r3].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r3].add (LogicalChannelNumberDescriptor (
        0x0301,  4,    // Canal+
        0x0302, 33,    // Canal+ Cinema
        0x0303, 32,    // Canal+ Sport
        0x0304, 35,    // Planete
        0x0305, 37,    // Canal J
        0x0306, 30,    // TPS Star
        -1));
    if (add_service_list) {
        nit->transports[r3].add (ServiceListDescriptor (
            0x0301, 0x01,  // Canal+
            0x0302, 0x01,  // Canal+ Cinema
            0x0303, 0x01,  // Canal+ Sport
            0x0304, 0x01,  // Planete
            0x0305, 0x01,  // Canal J
            0x0306, 0x01,  // TPS Star
            -1));
    }
    nit->transports[r3].add (terrestrial_delivery);

    // R4

    nit->transports[r4].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r4].add (LogicalChannelNumberDescriptor (
        0x0401,  6,    // M6
        0x0402,  9,    // W9
        0x0403, 11,    // NT1
        0x0404, 31,    // Paris Premiere
        0x0406, 34,    // Fake service (workaround for TV Numeric initial EMM scanning bug)
        0x0407, 57,    // Arte HD
        -1));
    nit->transports[r4].add (HDSimulcastLogicalChannelDescriptor (
        0x0401, 56,  // M6 -> M6 HD
        0x0407,  7,  // Arte HD -> Arte
        -1));
    if (add_service_list) {
        nit->transports[r4].add (ServiceListDescriptor (
            0x0401, 0x01,  // M6
            0x0402, 0x01,  // W9
            0x0403, 0x01,  // NT1
            0x0404, 0x01,  // Paris Premiere
            0x0406, 0x01,  // Fake service (workaround for TV Numeric initial EMM scanning bug)
            0x0407, 0x01,  // Arte HD
            -1));
    }
    nit->transports[r4].add (terrestrial_delivery);

    // R5

    nit->transports[r5].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r5].add (LogicalChannelNumberDescriptor (
        0x0501, 51,    // TF1 HD
        0x0502, 52,    // France 2 HD
        0x0503, 56,    // M6 HD
        -1));
    nit->transports[r5].add (HDSimulcastLogicalChannelDescriptor (
        0x0501, 1,  // TF1 HD -> TF1
        0x0502, 2,  // France 2 HD -> France 2
        0x0503, 6,  // M6 HD -> M6
        -1));
    if (add_service_list) {
        nit->transports[r5].add (ServiceListDescriptor (
            0x0501, 0x01,  // TF1 HD
            0x0502, 0x01,  // France 2 HD
            0x0503, 0x01,  // M6 HD
            -1));
    }
    nit->transports[r5].add (terrestrial_delivery);

    // R6

    nit->transports[r6].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[r6].add (LogicalChannelNumberDescriptor (
        0x0601,  1,    // TF1
        0x0602, 12,    // NRJ12
        0x0603, 38,    // LCI
        0x0604, 39,    // Eurosport France
        0x0605, 36,    // TF6
        0x0606, 10,    // TMC
        -1));
    nit->transports[r6].add (HDSimulcastLogicalChannelDescriptor (
        0x0601, 51,  // TF1 -> TF1 HD
        -1));
    if (add_service_list) {
        nit->transports[r6].add (ServiceListDescriptor (
            0x0601, 0x01,  // TF1
            0x0602, 0x01,  // NRJ12
            0x0603, 0x01,  // LCI
            0x0604, 0x01,  // Eurosport France
            0x0605, 0x01,  // TF6
            0x0606, 0x01,  // TMC
            -1));
    }
    nit->transports[r6].add (terrestrial_delivery);

    // L8

    nit->transports[l8].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    nit->transports[l8].add (LogicalChannelNumberDescriptor (
        0x0801, 20,    // Locale LCN 20
        0x0802, 21,    // Locale LCN 21
        0x0803, 22,    // Locale LCN 22
        0x0804, 23,    // Locale LCN 23
        0x0805, 24,    // Locale LCN 24
        0x0806, 25,    // Locale LCN 25
        0x0883, 23,    // France 3 LCN 23
        -1));
    if (add_service_list) {
        nit->transports[l8].add (ServiceListDescriptor (
            0x0801, 0x01,  // Locale LCN 20
            0x0802, 0x01,  // Locale LCN 21
            0x0803, 0x01,  // Locale LCN 22
            0x0804, 0x01,  // Locale LCN 23
            0x0805, 0x01,  // Locale LCN 24
            0x0806, 0x01,  // Locale LCN 25
            0x0883, 0x01,  // France 3 LCN 23
            -1));
    }
    nit->transports[l8].add (terrestrial_delivery);
}


//----------------------------------------------------------------------------
// Table Generation - NIT version 24 - oct 2007
//----------------------------------------------------------------------------

void ts::TNTNITPlugin::generateV24 (AbstractTablePtr& table)
{
    // Create table

    NIT* nit = new NIT();
    table = nit;

    // Transport stream id

    TransportStreamId r1 (1, NID_TNT_FRANCE);
    TransportStreamId r2 (2, NID_TNT_FRANCE);
    TransportStreamId r3 (3, NID_TNT_FRANCE);
    TransportStreamId r4 (4, NID_TNT_FRANCE);
    TransportStreamId r5 (5, NID_TNT_FRANCE);
    TransportStreamId r6 (6, NID_TNT_FRANCE);
    TransportStreamId l8 (8, NID_TNT_FRANCE);

    // Fixed fields

    nit->version = 24;
    nit->is_current = true;
    nit->network_id = NID_TNT_FRANCE;

    // All terrestrial delivery descriptors are generic

    TerrestrialDeliverySystemDescriptor terrestrial_delivery;
    terrestrial_delivery.centre_frequency = 0xFFFFFFFF;
    terrestrial_delivery.bandwidth = 0;          // 8 Mhz
    terrestrial_delivery.high_priority = true;
    terrestrial_delivery.no_time_slicing = true;
    terrestrial_delivery.no_mpe_fec = true;
    terrestrial_delivery.constellation = 2;      // 64-QAM
    terrestrial_delivery.hierarchy = 0;          // non-hierarchical, native interleaver
    terrestrial_delivery.code_rate_hp = 7;       // reserved
    terrestrial_delivery.code_rate_lp = 0;       // 1/2
    terrestrial_delivery.guard_interval = 0;     // 1/32
    terrestrial_delivery.transmission_mode = 1;  // 8K
    terrestrial_delivery.other_frequency = false;

    // Main descriptor loop
    // Note: SSU linkage_descriptor for L8 removed from v24

    nit->descs.add (NetworkNameDescriptor (TNT_OLD_NAME_LATIN1));
    nit->descs.add (SSULinkageDescriptor (1, nit->network_id, 0x01FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (2, nit->network_id, 0x02FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (3, nit->network_id, 0x03FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (4, nit->network_id, 0x04FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (5, nit->network_id, 0x05FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (6, nit->network_id, 0x06FF, OUI_DVB));

    // R1

    nit->transports[r1].add (terrestrial_delivery);
    if (add_service_list) {
        nit->transports[r1].add (ServiceListDescriptor (
            0x0101, 0x01,  // France 2
            0x0104, 0x01,  // France 5
            0x0105, 0x01,  // Arte
            0x0106, 0x01,  // LCP
            0x0111, 0x01,  // France 3a
            0x0112, 0x01,  // France 3b
            0x0113, 0x01,  // France 3c
            0x0114, 0x01,  // France 3
            0x0115, 0x01,  // France 3
            0x0116, 0x01,  // France 3
            0x0117, 0x01,  // France 3
            0x0118, 0x01,  // France 3 Poitiers
            0x0119, 0x01,  // France 3
            0x011A, 0x01,  // France 3 Toulouse
            0x011B, 0x01,  // France 3 Rhone
            0x011C, 0x01,  // France 3
            0x011D, 0x01,  // France 3
            0x011E, 0x01,  // France 3 Berry
            0x011F, 0x01,  // France 3
            0x0120, 0x01,  // France 3
            0x0121, 0x01,  // France 3
            0x0122, 0x01,  // France 3 Le Havre
            0x0123, 0x01,  // France 3
            0x0124, 0x01,  // France 3
            0x0125, 0x01,  // France 3 Orleans
            0x0126, 0x01,  // France 3
            0x0127, 0x01,  // France 3
            0x0128, 0x01,  // France 3
            0x0129, 0x01,  // France 3
            0x012A, 0x01,  // France 3
            0x012B, 0x01,  // France 3
            0x012C, 0x01,  // France 3
            0x012D, 0x01,  // France 3
            0x012E, 0x01,  // France 3
            0x012F, 0x01,  // France 3
            0x0130, 0x01,  // France 3
            0x0131, 0x01,  // France 3
            0x0132, 0x01,  // France 3 Dijon
            0x0133, 0x01,  // France 3 Montpellier
            0x0134, 0x01,  // France 3
            0x0135, 0x01,  // France 3
            0x0136, 0x01,  // France 3
            0x0137, 0x01,  // France 3
            0x0138, 0x01,  // France 3 Provence
            0x0139, 0x01,  // France 3
            0x013A, 0x01,  // France 3 Champardenne
            0x013B, 0x01,  // France 3
            0x0143, 0x01,  // France 3 LCN 22 a
            0x0144, 0x01,  // France 3 LCN 22 b
            0x0170, 0x01,  // Locale LCN 20
            0x0171, 0x01,  // Locale LCN 21
            0x0172, 0x01,  // Locale LCN 22
            0x0173, 0x01,  // Locale LCN 23
            0x0174, 0x01,  // Locale LCN 24
            0x0175, 0x01,  // Locale LCN 25
            0x0176, 0x01,  // France O
            -1));
    }
    nit->transports[r1].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r1].add (LogicalChannelNumberDescriptor (
            0x0101,  2,    // France 2
            0x0105,  7,    // Arte
            0x0111,  3,    // France 3a
            0x0113,  3,    // France 3c
            0x0114,  3,    // France 3
            0x0115,  3,    // France 3
            0x0116,  3,    // France 3
            0x0117,  3,    // France 3
            0x0118,  3,    // France 3 Poitiers
            0x0119,  3,    // France 3
            0x011A,  3,    // France 3 Toulouse
            0x011B,  3,    // France 3 Rhone
            0x011C,  3,    // France 3
            0x011D,  3,    // France 3
            0x0170, 20,    // Locale LCN 20
            0x0171, 21,    // Locale LCN 21
            0x0172, 22,    // Locale LCN 22
            0x0173, 23,    // Locale LCN 23
            0x0174, 24,    // Locale LCN 24
            0x0175, 25,    // Locale LCN 25
            0x0176, 20,    // France O
            -1));
        nit->transports[r1].add (HDSimulcastLogicalChannelDescriptor (
            0x0101, 52,  // France 2 -> France 2 HD
            -1));
        nit->transports[r1].add (LogicalChannelNumberDescriptor (
            0x0104,  5,    // France 5
            0x0106, 13,    // LCP
            0x0112,  3,    // France 3b
            0x011E,  3,    // France 3 Berry
            0x011F,  3,    // France 3
            0x0120,  3,    // France 3
            0x0121,  3,    // France 3
            0x0122,  3,    // France 3 Le Havre
            0x0123,  3,    // France 3
            0x0124,  3,    // France 3
            0x0125,  3,    // France 3 Orleans
            0x0126,  3,    // France 3
            0x0127,  3,    // France 3
            0x0128,  3,    // France 3
            0x0129,  3,    // France 3
            0x012A,  3,    // France 3
            0x012B,  3,    // France 3
            0x012C,  3,    // France 3
            0x012D,  3,    // France 3
            0x012E,  3,    // France 3
            0x012F,  3,    // France 3
            0x0130,  3,    // France 3
            0x0131,  3,    // France 3
            0x0132,  3,    // France 3 Dijon
            0x0133,  3,    // France 3 Montpellier
            0x0134,  3,    // France 3
            0x0135,  3,    // France 3
            0x0136,  3,    // France 3
            0x0137,  3,    // France 3
            0x0138,  3,    // France 3 Provence
            0x0139,  3,    // France 3
            0x013A,  3,    // France 3 Champardenne
            0x013B,  3,    // France 3
            0x0143, 22,    // France 3 LCN 22 a
            0x0144, 22,    // France 3 LCN 22 b
            -1));
        nit->transports[r1].add (HDSimulcastLogicalChannelDescriptor (
            0x0105, 57,  // Arte -> Arte HD
            -1));
    }
    else {
        nit->transports[r1].add (LogicalChannelNumberDescriptor (
            0x0101,  2,    // France 2
            0x0104,  5,    // France 5
            0x0105,  7,    // Arte
            0x0106, 13,    // LCP
            0x0111,  3,    // France 3a
            0x0112,  3,    // France 3b
            0x0113,  3,    // France 3c
            0x0114,  3,    // France 3
            0x0115,  3,    // France 3
            0x0116,  3,    // France 3
            0x0117,  3,    // France 3
            0x0118,  3,    // France 3 Poitiers
            0x0119,  3,    // France 3
            0x011A,  3,    // France 3 Toulouse
            0x011B,  3,    // France 3 Rhone
            0x011C,  3,    // France 3
            0x011D,  3,    // France 3
            0x011E,  3,    // France 3 Berry
            0x011F,  3,    // France 3
            0x0120,  3,    // France 3
            0x0121,  3,    // France 3
            0x0122,  3,    // France 3 Le Havre
            0x0123,  3,    // France 3
            0x0124,  3,    // France 3
            0x0125,  3,    // France 3 Orleans
            0x0126,  3,    // France 3
            0x0127,  3,    // France 3
            0x0128,  3,    // France 3
            0x0129,  3,    // France 3
            0x012A,  3,    // France 3
            0x012B,  3,    // France 3
            0x012C,  3,    // France 3
            0x012D,  3,    // France 3
            0x012E,  3,    // France 3
            0x012F,  3,    // France 3
            0x0130,  3,    // France 3
            0x0131,  3,    // France 3
            0x0132,  3,    // France 3 Dijon
            0x0133,  3,    // France 3 Montpellier
            0x0134,  3,    // France 3
            0x0135,  3,    // France 3
            0x0136,  3,    // France 3
            0x0137,  3,    // France 3
            0x0138,  3,    // France 3 Provence
            0x0139,  3,    // France 3
            0x013A,  3,    // France 3 Champardenne
            0x013B,  3,    // France 3
            0x0143, 22,    // France 3 LCN 22 a
            0x0144, 22,    // France 3 LCN 22 b
            0x0170, 20,    // Locale LCN 20
            0x0171, 21,    // Locale LCN 21
            0x0172, 22,    // Locale LCN 22
            0x0173, 23,    // Locale LCN 23
            0x0174, 24,    // Locale LCN 24
            0x0175, 25,    // Locale LCN 25
            0x0176, 20,    // France O
            -1));
        nit->transports[r1].add (HDSimulcastLogicalChannelDescriptor (
            0x0101, 52,  // France 2 -> France 2 HD
            0x0105, 57,  // Arte -> Arte HD
            -1));
    }

    // R2

    nit->transports[r2].add (terrestrial_delivery);
    if (add_service_list) {
        nit->transports[r2].add (ServiceListDescriptor (
            0x0201, 0x01,  // Direct 8
            0x0203, 0x01,  // BFM TV
            0x0204, 0x01,  // i> Tele
            0x0205, 0x01,  // Europe 2 TV / Virgin 17
            0x0206, 0x01,  // Gulli
            0x0207, 0x01,  // France 4
            -1));
    }
    nit->transports[r2].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r2].add (LogicalChannelNumberDescriptor (
            0x0201,  8,    // Direct 8
            0x0204, 16,    // i> Tele
            0x0206, 18,    // Gulli
            -1));
        nit->transports[r2].add (LogicalChannelNumberDescriptor (
            0x0203, 15,    // BFM TV
            0x0205, 17,    // Europe 2 TV / Virgin 17
            0x0207, 14,    // France 4
            -1));
    }
    else {
        nit->transports[r2].add (LogicalChannelNumberDescriptor (
            0x0201,  8,    // Direct 8
            0x0203, 15,    // BFM TV
            0x0204, 16,    // i> Tele
            0x0205, 17,    // Europe 2 TV / Virgin 17
            0x0206, 18,    // Gulli
            0x0207, 14,    // France 4
            -1));
    }

    // R3

    nit->transports[r3].add (terrestrial_delivery);
    if (add_service_list) {
        nit->transports[r3].add (ServiceListDescriptor (
            0x0301, 0x01,  // Canal+
            0x0302, 0x01,  // Canal+ Cinema
            0x0303, 0x01,  // Canal+ Sport
            0x0304, 0x01,  // Planete
            0x0305, 0x01,  // Canal J
            0x0306, 0x01,  // TPS Star
            -1));
    }
    nit->transports[r3].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r3].add (LogicalChannelNumberDescriptor (
            0x0301,  4,    // Canal+
            0x0303, 32,    // Canal+ Sport
            0x0305, 37,    // Canal J
            -1));
        nit->transports[r3].add (LogicalChannelNumberDescriptor (
            0x0302, 33,    // Canal+ Cinema
            0x0304, 35,    // Planete
            0x0306, 30,    // TPS Star
            -1));
    }
    else {
        nit->transports[r3].add (LogicalChannelNumberDescriptor (
            0x0301,  4,    // Canal+
            0x0302, 33,    // Canal+ Cinema
            0x0303, 32,    // Canal+ Sport
            0x0304, 35,    // Planete
            0x0305, 37,    // Canal J
            0x0306, 30,    // TPS Star
            -1));
    }

    // R4

    nit->transports[r4].add (terrestrial_delivery);
    if (add_service_list) {
        nit->transports[r4].add (ServiceListDescriptor (
            0x0401, 0x01,  // M6
            0x0402, 0x01,  // W9
            0x0403, 0x01,  // NT1
            0x0404, 0x01,  // Paris Premiere
            0x0405, 0x01,  // TF6
            0x0406, 0x01,  // AB1
            0x0407, 0x19,  // Arte HD
            -1));
    }
    nit->transports[r4].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r4].add (LogicalChannelNumberDescriptor (
            0x0401,  6,    // M6
            0x0403, 11,    // NT1
            0x0405, 36,    // TF6
            0x0407, 57,    // Arte HD
            -1));
        nit->transports[r4].add (HDSimulcastLogicalChannelDescriptor (
            0x0401, 56,  // M6 -> M6 HD
            -1));
        nit->transports[r4].add (LogicalChannelNumberDescriptor (
            0x0402,  9,    // W9
            0x0404, 31,    // Paris Premiere
            0x0406, 34,    // AB1
            -1));
        nit->transports[r4].add (HDSimulcastLogicalChannelDescriptor (
            0x0407,  7,  // Arte HD -> Arte
            -1));
    }
    else {
        nit->transports[r4].add (LogicalChannelNumberDescriptor (
            0x0401,  6,    // M6
            0x0402,  9,    // W9
            0x0403, 11,    // NT1
            0x0404, 31,    // Paris Premiere
            0x0405, 36,    // TF6
            0x0406, 34,    // AB1
            0x0407, 57,    // Arte HD
            -1));
        nit->transports[r4].add (HDSimulcastLogicalChannelDescriptor (
            0x0401, 56,  // M6 -> M6 HD
            0x0407,  7,  // Arte HD -> Arte
            -1));
    }

    // R5

    nit->transports[r5].add (terrestrial_delivery);
    if (add_service_list) {
        nit->transports[r5].add (ServiceListDescriptor (
            0x0501, 0x19,  // TF1 HD
            0x0502, 0x19,  // France 2 HD
            0x0503, 0x19,  // M6 HD
            -1));
    }
    nit->transports[r5].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r5].add (LogicalChannelNumberDescriptor (
            0x0501, 50,    // TF1 HD
            0x0503, 56,    // M6 HD
            -1));
        nit->transports[r5].add (HDSimulcastLogicalChannelDescriptor (
            0x0503, 6,  // M6 HD -> M6
            -1));
        nit->transports[r5].add (LogicalChannelNumberDescriptor (
            0x0502, 51,    // France 2 HD
            -1));
        nit->transports[r5].add (HDSimulcastLogicalChannelDescriptor (
            0x0501, 1,  // TF1 HD -> TF1
            0x0502, 2,  // France 2 HD -> France 2
            -1));
    }
    else {
        nit->transports[r5].add (LogicalChannelNumberDescriptor (
            0x0501, 50,    // TF1 HD
            0x0502, 51,    // France 2 HD
            0x0503, 56,    // M6 HD
            -1));
        nit->transports[r5].add (HDSimulcastLogicalChannelDescriptor (
            0x0501, 1,  // TF1 HD -> TF1
            0x0502, 2,  // France 2 HD -> France 2
            0x0503, 6,  // M6 HD -> M6
            -1));
    }

    // R6

    nit->transports[r6].add (terrestrial_delivery);
    if (add_service_list) {
        nit->transports[r6].add (ServiceListDescriptor (
            0x0601, 0x01,  // TF1
            0x0602, 0x01,  // NRJ12
            0x0603, 0x01,  // LCI
            0x0604, 0x01,  // Eurosport France
            0x0606, 0x01,  // TMC
            -1));
    }
    nit->transports[r6].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r6].add (LogicalChannelNumberDescriptor (
            0x0601,  1,    // TF1
            0x0603, 38,    // LCI
            -1));
        nit->transports[r6].add (HDSimulcastLogicalChannelDescriptor (
            0x0601, 51,  // TF1 -> TF1 HD
            -1));
        nit->transports[r6].add (LogicalChannelNumberDescriptor (
            0x0602, 12,    // NRJ12
            0x0604, 39,    // Eurosport France
            0x0606, 10,    // TMC
            -1));
    }
    else {
        nit->transports[r6].add (LogicalChannelNumberDescriptor (
            0x0601,  1,    // TF1
            0x0602, 12,    // NRJ12
            0x0603, 38,    // LCI
            0x0604, 39,    // Eurosport France
            0x0606, 10,    // TMC
            -1));
        nit->transports[r6].add (HDSimulcastLogicalChannelDescriptor (
            0x0601, 51,  // TF1 -> TF1 HD
            -1));
    }

    // L8

    nit->transports[l8].add (terrestrial_delivery);
    if (add_service_list) {
        nit->transports[l8].add (ServiceListDescriptor (
            0x0801, 0x01,  // Locale LCN 20
            0x0802, 0x01,  // Locale LCN 21
            0x0803, 0x01,  // Locale LCN 22
            0x0804, 0x01,  // Locale LCN 23
            0x0805, 0x01,  // Locale LCN 24
            0x0806, 0x01,  // Locale LCN 25
            0x0883, 0x01,  // France 3 LCN 23
            -1));
    }
    nit->transports[l8].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[l8].add (LogicalChannelNumberDescriptor (
            0x0805, 24,    // Locale LCN 24
            0x0806, 25,    // Locale LCN 25
            0x0883, 23,    // France 3 LCN 23
            -1));
        nit->transports[l8].add (LogicalChannelNumberDescriptor (
            0x0801, 20,    // Locale LCN 20
            0x0802, 21,    // Locale LCN 21
            0x0803, 22,    // Locale LCN 22
            0x0804, 23,    // Locale LCN 23
            -1));
    }
    else {
        nit->transports[l8].add (LogicalChannelNumberDescriptor (
            0x0801, 20,    // Locale LCN 20
            0x0802, 21,    // Locale LCN 21
            0x0803, 22,    // Locale LCN 22
            0x0804, 23,    // Locale LCN 23
            0x0805, 24,    // Locale LCN 24
            0x0806, 25,    // Locale LCN 25
            0x0883, 23,    // France 3 LCN 23
            -1));
    }
}


//----------------------------------------------------------------------------
// Table Generation - NIT version 23 - sep 2007
//----------------------------------------------------------------------------

void ts::TNTNITPlugin::generateV23 (AbstractTablePtr& table)
{
    // Create table

    NIT* nit = new NIT();
    table = nit;

    // Transport stream id

    TransportStreamId r1 (1, NID_TNT_FRANCE);
    TransportStreamId r2 (2, NID_TNT_FRANCE);
    TransportStreamId r3 (3, NID_TNT_FRANCE);
    TransportStreamId r4 (4, NID_TNT_FRANCE);
    TransportStreamId r5 (5, NID_TNT_FRANCE);
    TransportStreamId r6 (6, NID_TNT_FRANCE);
    TransportStreamId l8 (8, NID_TNT_FRANCE);

    // Fixed fields

    nit->version = 23;
    nit->is_current = true;
    nit->network_id = NID_TNT_FRANCE;

    // All terrestrial delivery descriptors are generic

    TerrestrialDeliverySystemDescriptor terrestrial_delivery;
    terrestrial_delivery.centre_frequency = 0xFFFFFFFF;
    terrestrial_delivery.bandwidth = 0;          // 8 Mhz
    terrestrial_delivery.high_priority = true;
    terrestrial_delivery.no_time_slicing = true;
    terrestrial_delivery.no_mpe_fec = true;
    terrestrial_delivery.constellation = 2;      // 64-QAM
    terrestrial_delivery.hierarchy = 0;          // non-hierarchical, native interleaver
    terrestrial_delivery.code_rate_hp = 7;       // reserved
    terrestrial_delivery.code_rate_lp = 0;       // 1/2
    terrestrial_delivery.guard_interval = 0;     // 1/32
    terrestrial_delivery.transmission_mode = 1;  // 8K
    terrestrial_delivery.other_frequency = false;

    // Main descriptor loop

    nit->descs.add (NetworkNameDescriptor (TNT_OLD_NAME_LATIN1));
    nit->descs.add (SSULinkageDescriptor (1, nit->network_id, 0x01FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (2, nit->network_id, 0x02FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (3, nit->network_id, 0x03FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (4, nit->network_id, 0x04FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (5, nit->network_id, 0x05FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (6, nit->network_id, 0x06FF, OUI_DVB));
    nit->descs.add (SSULinkageDescriptor (8, nit->network_id, 0x08FF, OUI_DVB));

    // R1

    nit->transports[r1].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r1].add (LogicalChannelNumberDescriptor (
            0x0101,  2,    // France 2
            0x0105,  7,    // Arte
            0x0112,  3,    // France 3b
            0x0143, 22,    // France 3 LCN 22 a
            0x0170, 20,    // Locale LCN 20
            0x0171, 21,    // Locale LCN 21
            0x0172, 22,    // Locale LCN 22
            0x0173, 23,    // Locale LCN 23
            0x0174, 24,    // Locale LCN 24
            0x0175, 25,    // Locale LCN 25
            0x0176, 20,    // France O
            -1));
        nit->transports[r1].add (LogicalChannelNumberDescriptor (
            0x0104,  5,    // France 5
            0x0106, 13,    // LCP
            0x0111,  3,    // France 3a
            0x0113,  3,    // France 3c
            0x0114,  3,    // France 3
            0x0115,  3,    // France 3
            0x0116,  3,    // France 3
            0x0117,  3,    // France 3
            0x0118,  3,    // France 3 Poitiers
            0x0119,  3,    // France 3
            0x011A,  3,    // France 3 Toulouse
            0x011B,  3,    // France 3 Rhone
            0x011C,  3,    // France 3
            0x011D,  3,    // France 3
            0x011E,  3,    // France 3 Berry
            0x011F,  3,    // France 3
            0x0120,  3,    // France 3
            0x0121,  3,    // France 3
            0x0122,  3,    // France 3 Le Havre
            0x0123,  3,    // France 3
            0x0124,  3,    // France 3
            0x0125,  3,    // France 3 Orleans
            0x0126,  3,    // France 3
            0x0127,  3,    // France 3
            0x0128,  3,    // France 3
            0x0129,  3,    // France 3
            0x012A,  3,    // France 3
            0x012B,  3,    // France 3
            0x012C,  3,    // France 3
            0x012D,  3,    // France 3
            0x012E,  3,    // France 3
            0x012F,  3,    // France 3
            0x0130,  3,    // France 3
            0x0131,  3,    // France 3
            0x0132,  3,    // France 3 Dijon
            0x0133,  3,    // France 3 Montpellier
            0x0134,  3,    // France 3
            0x0135,  3,    // France 3
            0x0136,  3,    // France 3
            0x0137,  3,    // France 3
            0x0138,  3,    // France 3 Provence
            0x0139,  3,    // France 3
            0x013A,  3,    // France 3 Champardenne
            0x013B,  3,    // France 3
            0x0144, 22,    // France 3 LCN 22 b
            -1));
    }
    else {
        nit->transports[r1].add (LogicalChannelNumberDescriptor (
            0x0101,  2,    // France 2
            0x0104,  5,    // France 5
            0x0105,  7,    // Arte
            0x0106, 13,    // LCP
            0x0111,  3,    // France 3a
            0x0112,  3,    // France 3b
            0x0113,  3,    // France 3c
            0x0114,  3,    // France 3
            0x0115,  3,    // France 3
            0x0116,  3,    // France 3
            0x0117,  3,    // France 3
            0x0118,  3,    // France 3 Poitiers
            0x0119,  3,    // France 3
            0x011A,  3,    // France 3 Toulouse
            0x011B,  3,    // France 3 Rhone
            0x011C,  3,    // France 3
            0x011D,  3,    // France 3
            0x011E,  3,    // France 3 Berry
            0x011F,  3,    // France 3
            0x0120,  3,    // France 3
            0x0121,  3,    // France 3
            0x0122,  3,    // France 3 Le Havre
            0x0123,  3,    // France 3
            0x0124,  3,    // France 3
            0x0125,  3,    // France 3 Orleans
            0x0126,  3,    // France 3
            0x0127,  3,    // France 3
            0x0128,  3,    // France 3
            0x0129,  3,    // France 3
            0x012A,  3,    // France 3
            0x012B,  3,    // France 3
            0x012C,  3,    // France 3
            0x012D,  3,    // France 3
            0x012E,  3,    // France 3
            0x012F,  3,    // France 3
            0x0130,  3,    // France 3
            0x0131,  3,    // France 3
            0x0132,  3,    // France 3 Dijon
            0x0133,  3,    // France 3 Montpellier
            0x0134,  3,    // France 3
            0x0135,  3,    // France 3
            0x0136,  3,    // France 3
            0x0137,  3,    // France 3
            0x0138,  3,    // France 3 Provence
            0x0139,  3,    // France 3
            0x013A,  3,    // France 3 Champardenne
            0x013B,  3,    // France 3
            0x0143, 22,    // France 3 LCN 22 a
            0x0144, 22,    // France 3 LCN 22 b
            0x0170, 20,    // Locale LCN 20
            0x0171, 21,    // Locale LCN 21
            0x0172, 22,    // Locale LCN 22
            0x0173, 23,    // Locale LCN 23
            0x0174, 24,    // Locale LCN 24
            0x0175, 25,    // Locale LCN 25
            0x0176, 20,    // France O
            -1));
    }
    if (add_service_list) {
        nit->transports[r1].add (ServiceListDescriptor (
            0x0101, 0x01,  // France 2
            0x0104, 0x01,  // France 5
            0x0105, 0x01,  // Arte
            0x0106, 0x01,  // LCP
            0x0111, 0x01,  // France 3a
            0x0112, 0x01,  // France 3b
            0x0113, 0x01,  // France 3c
            0x0114, 0x01,  // France 3
            0x0115, 0x01,  // France 3
            0x0116, 0x01,  // France 3
            0x0117, 0x01,  // France 3
            0x0118, 0x01,  // France 3 Poitiers
            0x0119, 0x01,  // France 3
            0x011A, 0x01,  // France 3 Toulouse
            0x011B, 0x01,  // France 3 Rhone
            0x011C, 0x01,  // France 3
            0x011D, 0x01,  // France 3
            0x011E, 0x01,  // France 3 Berry
            0x011F, 0x01,  // France 3
            0x0120, 0x01,  // France 3
            0x0121, 0x01,  // France 3
            0x0122, 0x01,  // France 3 Le Havre
            0x0123, 0x01,  // France 3
            0x0124, 0x01,  // France 3
            0x0125, 0x01,  // France 3 Orleans
            0x0126, 0x01,  // France 3
            0x0127, 0x01,  // France 3
            0x0128, 0x01,  // France 3
            0x0129, 0x01,  // France 3
            0x012A, 0x01,  // France 3
            0x012B, 0x01,  // France 3
            0x012C, 0x01,  // France 3
            0x012D, 0x01,  // France 3
            0x012E, 0x01,  // France 3
            0x012F, 0x01,  // France 3
            0x0130, 0x01,  // France 3
            0x0131, 0x01,  // France 3
            0x0132, 0x01,  // France 3 Dijon
            0x0133, 0x01,  // France 3 Montpellier
            0x0134, 0x01,  // France 3
            0x0135, 0x01,  // France 3
            0x0136, 0x01,  // France 3
            0x0137, 0x01,  // France 3
            0x0138, 0x01,  // France 3 Provence
            0x0139, 0x01,  // France 3
            0x013A, 0x01,  // France 3 Champardenne
            0x013B, 0x01,  // France 3
            0x0143, 0x01,  // France 3 LCN 22 a
            0x0144, 0x01,  // France 3 LCN 22 b
            0x0170, 0x01,  // Locale LCN 20
            0x0171, 0x01,  // Locale LCN 21
            0x0172, 0x01,  // Locale LCN 22
            0x0173, 0x01,  // Locale LCN 23
            0x0174, 0x01,  // Locale LCN 24
            0x0175, 0x01,  // Locale LCN 25
            0x0176, 0x01,  // France O
            -1));
    }
    nit->transports[r1].add (terrestrial_delivery);

    // R2

    nit->transports[r2].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r2].add (LogicalChannelNumberDescriptor (
            0x0201,  8,    // Direct 8
            0x0204, 16,    // i> Tele
            0x0206, 18,    // Gulli
            -1));
        nit->transports[r2].add (LogicalChannelNumberDescriptor (
            0x0203, 15,    // BFM TV
            0x0205, 17,    // Europe 2 TV / Virgin 17
            0x0207, 14,    // France 4
            -1));
    }
    else {
        nit->transports[r2].add (LogicalChannelNumberDescriptor (
            0x0201,  8,    // Direct 8
            0x0203, 15,    // BFM TV
            0x0204, 16,    // i> Tele
            0x0205, 17,    // Europe 2 TV / Virgin 17
            0x0206, 18,    // Gulli
            0x0207, 14,    // France 4
            -1));
    }
    if (add_service_list) {
        nit->transports[r2].add (ServiceListDescriptor (
            0x0201, 0x01,  // Direct 8
            0x0203, 0x01,  // BFM TV
            0x0204, 0x01,  // i> Tele
            0x0205, 0x01,  // Europe 2 TV / Virgin 17
            0x0206, 0x01,  // Gulli
            0x0207, 0x01,  // France 4
            -1));
    }
    nit->transports[r2].add (terrestrial_delivery);

    // R3

    nit->transports[r3].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r3].add (LogicalChannelNumberDescriptor (
            0x0301,  4,    // Canal+
            0x0303, 32,    // Canal+ Sport
            0x0305, 37,    // Canal J
            -1));
        nit->transports[r3].add (LogicalChannelNumberDescriptor (
            0x0302, 33,    // Canal+ Cinema
            0x0304, 35,    // Planete
            0x0306, 30,    // TPS Star
            -1));
    }
    else {
        nit->transports[r3].add (LogicalChannelNumberDescriptor (
            0x0301,  4,    // Canal+
            0x0302, 33,    // Canal+ Cinema
            0x0303, 32,    // Canal+ Sport
            0x0304, 35,    // Planete
            0x0305, 37,    // Canal J
            0x0306, 30,    // TPS Star
            -1));
    }
    if (add_service_list) {
        nit->transports[r3].add (ServiceListDescriptor (
            0x0301, 0x01,  // Canal+
            0x0302, 0x01,  // Canal+ Cinema
            0x0303, 0x01,  // Canal+ Sport
            0x0304, 0x01,  // Planete
            0x0305, 0x01,  // Canal J
            0x0306, 0x01,  // TPS Star
            -1));
    }
    nit->transports[r3].add (terrestrial_delivery);

    // R4

    nit->transports[r4].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r4].add (LogicalChannelNumberDescriptor (
            0x0401,  6,    // M6
            0x0403, 11,    // NT1
            0x0405, 36,    // TF6
            -1));
        nit->transports[r4].add (LogicalChannelNumberDescriptor (
            0x0402,  9,    // W9
            0x0404, 31,    // Paris Premiere
            0x0406, 34,    // AB1
            -1));
    }
    else {
        nit->transports[r4].add (LogicalChannelNumberDescriptor (
            0x0401,  6,    // M6
            0x0402,  9,    // W9
            0x0403, 11,    // NT1
            0x0404, 31,    // Paris Premiere
            0x0405, 36,    // TF6
            0x0406, 34,    // AB1
            -1));
    }
    if (add_service_list) {
        nit->transports[r4].add (ServiceListDescriptor (
            0x0401, 0x01,  // M6
            0x0402, 0x01,  // W9
            0x0403, 0x01,  // NT1
            0x0404, 0x01,  // Paris Premiere
            0x0405, 0x01,  // TF6
            0x0406, 0x01,  // AB1
            -1));
    }
    nit->transports[r4].add (terrestrial_delivery);

    // R5

    nit->transports[r5].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r5].add (LogicalChannelNumberDescriptor (
            0x0501, 50,    // Test HD1
            0x0503, 52,    // Test HD3
            -1));
        nit->transports[r5].add (LogicalChannelNumberDescriptor (
            0x0502, 51,    // Test HD2
            -1));
    }
    else {
        nit->transports[r5].add (LogicalChannelNumberDescriptor (
            0x0501, 50,    // Test HD1
            0x0502, 51,    // Test HD2
            0x0503, 52,    // Test HD3
            -1));
    }
    if (add_service_list) {
        nit->transports[r5].add (ServiceListDescriptor (
            0x0501, 0x19,  // Test HD1
            0x0502, 0x19,  // Test HD2
            0x0503, 0x19,  // Test HD3
            -1));
    }
    nit->transports[r5].add (terrestrial_delivery);

    // R6

    nit->transports[r6].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[r6].add (LogicalChannelNumberDescriptor (
            0x0601,  1,    // TF1
            0x0603, 38,    // LCI
            0x0606, 10,    // TMC
            -1));
        nit->transports[r6].add (LogicalChannelNumberDescriptor (
            0x0602, 12,    // NRJ12
            0x0604, 39,    // Eurosport France
            -1));
    }
    else {
        nit->transports[r6].add (LogicalChannelNumberDescriptor (
            0x0601,  1,    // TF1
            0x0602, 12,    // NRJ12
            0x0603, 38,    // LCI
            0x0604, 39,    // Eurosport France
            0x0606, 10,    // TMC
            -1));
    }
    if (add_service_list) {
        nit->transports[r6].add (ServiceListDescriptor (
            0x0601, 0x01,  // TF1
            0x0602, 0x01,  // NRJ12
            0x0603, 0x01,  // LCI
            0x0604, 0x01,  // Eurosport France
            0x0606, 0x01,  // TMC
            -1));
    }
    nit->transports[r6].add (terrestrial_delivery);

    // L8

    nit->transports[l8].add (PrivateDataSpecifierDescriptor (PDS_EICTA));
    if (split_lcn) {
        nit->transports[l8].add (LogicalChannelNumberDescriptor (
            0x0801, 20,    // Locale LCN 20
            0x0803, 22,    // Locale LCN 22
            0x0805, 24,    // Locale LCN 24
            0x0883, 23,    // France 3 LCN 23
            -1));
        nit->transports[l8].add (LogicalChannelNumberDescriptor (
            0x0802, 21,    // Locale LCN 21
            0x0804, 23,    // Locale LCN 23
            0x0806, 25,    // Locale LCN 25
            -1));
    }
    else {
        nit->transports[l8].add (LogicalChannelNumberDescriptor (
            0x0801, 20,    // Locale LCN 20
            0x0802, 21,    // Locale LCN 21
            0x0803, 22,    // Locale LCN 22
            0x0804, 23,    // Locale LCN 23
            0x0805, 24,    // Locale LCN 24
            0x0806, 25,    // Locale LCN 25
            0x0883, 23,    // France 3 LCN 23
            -1));
    }
    if (add_service_list) {
        nit->transports[l8].add (ServiceListDescriptor (
            0x0801, 0x01,  // Locale LCN 20
            0x0802, 0x01,  // Locale LCN 21
            0x0803, 0x01,  // Locale LCN 22
            0x0804, 0x01,  // Locale LCN 23
            0x0805, 0x01,  // Locale LCN 24
            0x0806, 0x01,  // Locale LCN 25
            0x0883, 0x01,  // France 3 LCN 23
            -1));
    }
    nit->transports[l8].add (terrestrial_delivery);
}
