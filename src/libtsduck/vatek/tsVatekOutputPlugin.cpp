//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Vision Advance Technology Inc. (VATek)
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

#if defined(TS_NO_VATEK)
#include "tsPlatform.h"
TS_LLVM_NOWARNING(missing-variable-declarations)
bool tsVatekOutputPluginIsEmpty = true; // Avoid warning about empty module.
#else

#include "tsVatekOutputPlugin.h"
#include "tsPluginRepository.h"

#include "tsBeforeStandardHeaders.h"
#include <vatek_sdk_usbstream.h>
#include <core/base/output_modulator.h>
#include <cross/cross_os_api.h>
#include "tsAfterStandardHeaders.h"

TS_REGISTER_OUTPUT_PLUGIN(u"vatek", ts::VatekOutputPlugin);


//----------------------------------------------------------------------------
// Class internals.
//----------------------------------------------------------------------------

class ts::VatekOutputPlugin::Guts
{
    TS_NOBUILD_NOCOPY(Guts);
public:
    // Constructor
    Guts(VatekOutputPlugin* plugin);

    enum tsvatek_bandwidth {
        tsvatek_bw_1_7 = 0,
        tsvatek_bw_5   = 5,
        tsvatek_bw_6   = 6,
        tsvatek_bw_7   = 7,
        tsvatek_bw_8   = 8,
        tsvatek_bw_10  = 10,
    };

    // Point to parent plugin.
    VatekOutputPlugin* const plugin;
    TSP* const tsp;

    // Plugin private working data.
    hvatek_devices   m_hdevices;
    hvatek_chip      m_hchip;
    hvatek_usbstream m_husbstream;
    usbstream_param  m_param;
    int32_t          m_index;
    Pusbstream_slice m_slicebuf;

    // Same as in parent plugin.
    bool start();
    bool stop();
    bool send(const TSPacket*, const TSPacketMetadata*, size_t);

    // Configuration by modulation type.
    void debugParams();
    vatek_result configParam();
    vatek_result modparam_config_dvb_t(Pmodulator_param pmod);
    vatek_result modparam_config_j83a(Pmodulator_param pmod);
    vatek_result modparam_config_atsc(Pmodulator_param pmod);
    vatek_result modparam_config_j83b(Pmodulator_param pmod);
    vatek_result modparam_config_dtmb(Pmodulator_param pmod);
    vatek_result modparam_config_isdb_t(Pmodulator_param pmod);
    vatek_result modparam_config_j83c(Pmodulator_param pmod);
    vatek_result modparam_config_dvb_t2(Pmodulator_param pmod);

    // Table of configuration methods per modulation.
    typedef vatek_result (VatekOutputPlugin::Guts::*fpmodparam_config)(Pmodulator_param pmod);
    struct tsmod_param_config {
        modulator_type    type;
        fpmodparam_config config;
    };
    static const tsmod_param_config m_modtables[];
};

ts::VatekOutputPlugin::Guts::Guts(VatekOutputPlugin* vop) :
    plugin(vop),
    tsp(vop->tsp),
    m_hdevices(nullptr),
    m_hchip(nullptr),
    m_husbstream(nullptr),
    m_param(),
    m_index(-1),
    m_slicebuf(nullptr)
{
    memset(&m_param, 0, sizeof(usbstream_param));
    m_param.r2param.freqkhz = 473000;
    m_param.mode = ustream_mode_async;
    m_param.remux = ustream_remux_pcr;
    m_param.pcradjust = pcr_disable;
    m_param.async.bitrate = 0;
    m_param.async.mode = uasync_mode_cbr;
    m_param.async.prepare_ms = 0;
}

const ts::VatekOutputPlugin::Guts::tsmod_param_config ts::VatekOutputPlugin::Guts::m_modtables[] =
{
    {modulator_dvb_t,  &VatekOutputPlugin::Guts::modparam_config_dvb_t},
    {modulator_j83a,   &VatekOutputPlugin::Guts::modparam_config_j83a},
    {modulator_atsc,   &VatekOutputPlugin::Guts::modparam_config_atsc},
    {modulator_j83b,   &VatekOutputPlugin::Guts::modparam_config_j83b},
    {modulator_dtmb,   &VatekOutputPlugin::Guts::modparam_config_dtmb},
    {modulator_isdb_t, &VatekOutputPlugin::Guts::modparam_config_isdb_t},
    {modulator_j83c,   &VatekOutputPlugin::Guts::modparam_config_j83c},
    {modulator_dvb_t2, &VatekOutputPlugin::Guts::modparam_config_dvb_t2},
};


//----------------------------------------------------------------------------
// Simple virtual methods.
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::isRealTime()
{
    return true;
}


//----------------------------------------------------------------------------
// Output plugin constructor
//----------------------------------------------------------------------------

ts::VatekOutputPlugin::VatekOutputPlugin(TSP* tsp_) :
    OutputPlugin(tsp_, u"Send packets to a VATek modulator device", u"[options]"),
    _guts(new Guts(this))
{
    CheckNonNull(_guts);

    option(u"device", 'd', UNSIGNED);
    help(u"device",
         u"Device index, from 0 to N-1 (with N being the number of VATek devices in the system). "
         u"Use the command \"tsvatek -a\" to have a complete list of devices in the system. "
         u"By default, use the first VATek device.");

    option(u"frequency", 'f', UNSIGNED);
    help(u"frequency",
         u"Indicate the frequency, in Hz, of the output carrier. "
         u"The valid range is 50,000,000 Hz to 1,000,000,000 Hz."
         u"The default is 473,000,000 Hz.");

    option(u"bandwidth", 0, Enumeration({
         {u"1.7", Guts::tsvatek_bw_1_7},
         {u"5",   Guts::tsvatek_bw_5},
         {u"6",   Guts::tsvatek_bw_6},
         {u"7",   Guts::tsvatek_bw_7},
         {u"8",   Guts::tsvatek_bw_8},
         {u"10",  Guts::tsvatek_bw_10},
    }));
    help(u"bandwidth",
         u"DVB-T, DVB-T2, DMB-T (DTMB): indicate bandwidth in MHz. "
         u"The default is 8 MHz. "
         u"The bandwidth values 1.7 and 10 MHz are valid for DVB-T2 only.");

    option(u"constellation", 0, Enumeration({
        {u"QPSK",   dvb_t_qpsk},
        {u"16-QAM", dvb_t_qam16},
        {u"64-QAM", dvb_t_qam64},
    }));
    help(u"constellation",
         u"DVB-T, ISDB-T: indicate the constellation type. The default is 64-QAM.");

    option(u"j83-qam", 0, Enumeration({
        {u"16-QAM",  j83a_qam16},
        {u"32-QAM",  j83a_qam32},
        {u"64-QAM",  j83a_qam64},
        {u"128-QAM", j83a_qam128},
        {u"256-QAM", j83a_qam256},
    }));
    help(u"j83-qam",
         u"The specified value and default depends on the modulation type.\n"
         u"J83A : 16-QAM, 32-QAM, 64-QAM, 128-QAM, 256-QAM. default 64-QAM.\n"
         u"J83B : 64-QAM, 256-QAM. default 256-QAM.\n"
         u"J83C : 64-QAM, 256-QAM. default 256-QAM.\n");

    option(u"convolutional-rate", 'r', Enumeration({
        {u"1/2",  coderate_1_2},
        {u"2/3",  coderate_2_3},
        {u"3/4",  coderate_3_4},
        {u"5/6",  coderate_5_6},
        {u"7/8",  coderate_7_8},
    }));
    help(u"convolutional-rate",
         u"Indicate the convolutional rate. "
         u"The specified value depends on the modulation type. "
         u"DVB-T,ISDB-T: 1/2, 2/3, 3/4, 5/6, 7/8. "
         u"The default is 5/6.");

    option(u"dmb-constellation", 0, Enumeration({
        {u"4-QAM-NR", dtmb_qam4_nr},
        {u"4-QAM",    dtmb_qpsk},
        {u"16-QAM",   dtmb_qam16},
        {u"32-QAM",   dtmb_qam32},
        {u"64-QAM",   dtmb_qam64},
    }));
    help(u"dmb-constellation",
         u"DMB-T (DTMB): indicate the constellation type. The default is 64-QAM.");

    option(u"dmb-fec", 0, Enumeration({
        {u"0.4", dtmb_code_rate_0_4},
        {u"0.6", dtmb_code_rate_0_6},
        {u"0.8", dtmb_code_rate_0_8},
    }));
    help(u"dmb-fec",
         u"DMB-T (DTMB): indicate the FEC code rate. The default is 0.8. "
         u"4-QAM-NR and 32-QAM can be used only with --dmb-fec 0.8.");

    option(u"dmb-carrier", 0, Enumeration({
        {u"1",    dtmb_carrier_1},
        {u"3780", dtmb_carrier_3780},
    }));
    help(u"dmb-carrier",
         u"DMB-T (DTMB): indicate the carrier mode. The default is 3780. ");

    option(u"dmb-frame-numbering");
    help(u"dmb-frame-numbering",
         u"DMB-T/H, ADTB-T: indicate to use frame numbering. The default "
         u"is to use no frame numbering.");

    option(u"dmb-header", 0, Enumeration({
        {u"PN420", dtmb_framesync_420},
        {u"PN595", dtmb_framesync_595},
        {u"PN945", dtmb_framesync_945},
    }));
    help(u"dmb-header",
         u"DMB-T/H, ADTB-T: indicate the FEC frame header mode. "
         u"The default is PN945.");

    option(u"dmb-interleaver", 0, Enumeration({
        {u"1", dtmb_interleaved_240},
        {u"2", dtmb_interleaved_720},
    }));
    help(u"dmb-interleaver",
         u"DMB-T (DTMB): indicate the interleaver mode. Must be one "
         u"1 (B=54, M=240) or 2 (B=54, M=720). The default is 1.");

    option(u"guard-interval", 'g', Enumeration({
        {u"1/32", guard_interval_1_32},
        {u"1/16", guard_interval_1_16},
        {u"1/8",  guard_interval_1_8},
        {u"1/4",  guard_interval_1_4},
    }));
    help(u"guard-interval", u"DVB-T and ISDB-T modulators: indicate the guard interval. The default is 1/16.");

    option(u"fft-mode", 0, Enumeration({
        {u"1K",  t2_fft_1k},
        {u"2K",  t2_fft_2k},
        {u"4K",  t2_fft_4k},
        {u"8K",  t2_fft_8k},
        {u"16K", t2_fft_16k},
    }));
    help(u"fft-mode", u"DVB-T2: indicate the FFT mode. The default is 32K.");

    option(u"pilots", 0);
    help(u"pilots", u"DVB-S2 and ADTB-T: enable pilots (default: no pilot).");

    option(u"pilot-pattern", 'p', Enumeration({
        {u"1", pilot_pattern_1},
        {u"2", pilot_pattern_2},
        {u"3", pilot_pattern_3},
        {u"4", pilot_pattern_4},
        {u"5", pilot_pattern_5},
        {u"6", pilot_pattern_6},
        {u"7", pilot_pattern_7},
        {u"8", pilot_pattern_8},
    }));
    help(u"pilot-pattern",
         u"DVB-T2: indicate the pilot pattern to use, a value in the range 1 to 8. The default is 7.");

    option(u"plp0-code-rate", 0, Enumeration({
        {u"1/2", t2_coderate_1_2},
        {u"3/5", t2_coderate_3_5},
        {u"2/3", t2_coderate_2_3},
        {u"3/4", t2_coderate_3_4},
        {u"4/5", t2_coderate_4_5},
        {u"5/6", t2_coderate_5_6},
    }));
    help(u"plp0-code-rate",
         u"DVB-T2: indicate the convolutional coding rate used by the PLP #0. The default is 2/3.");

    option(u"plp0-fec-type", 0, Enumeration({
        {u"16K", t2_fec_16200},
        {u"64K", t2_fec_64800},
    }));
    help(u"plp0-fec-type", u"DVB-T2: indicate the FEC type used by the PLP #0. The default is 64K LPDC.");

    option(u"plp0-issy", 0, Enumeration({
        {u"NONE",  t2_issy_disable},
        {u"SHORT", t2_issy_short},
        {u"LONG",  t2_issy_long},
    }));
    help(u"plp0-issy",
         u"DVB-T2: type of ISSY field to compute and insert in PLP #0. "
         u"The default is NONE.");

    option(u"t2-version", 0, Enumeration({
        {u"ver131",      t2_ver_131},
        {u"ver131_lite", t2_ver_131_lite,},
    }));
    help(u"t2-version", u"DVB-T2: version tag. The default is ver131.");

    option(u"plp0-high-efficiency");
    help(u"plp0-high-efficiency",
         u"DVB-T2: indicate that the PLP #0 uses High Efficiency Mode (HEM). "
         u"Otherwise Normal Mode (NM) is used.");

    option(u"plp0-modulation", 0, Enumeration({
        {u"QPSK",    t2_plp_qpsk},
        {u"16-QAM",  t2_plp_qam16},
        {u"64-QAM",  t2_plp_qam64},
        {u"256-QAM", t2_plp_qam256},
    }));
    help(u"plp0-modulation",
         u"DVB-T2: indicate the modulation used by PLP #0. The default is 256-QAM.");

    option(u"plp0-null-packet-deletion");
    help(u"plp0-null-packet-deletion",
         u"DVB-T2: indicate that null-packet deletion is active in PLP #0. "
         u"Otherwise it is not active.");

    option(u"plp0-rotation");
    help(u"plp0-rotation",
         u"DVB-T2: indicate that constellation rotation is used for PLP #0. Otherwise not.");

    option(u"symbol-rate", 0, POSITIVE);
    help(u"symbol-rate",
         u"J83a: Specify the symbol rate in symbols/second. "
         u"The default is 5,120,000 sym/s");

    option(u"t2-guard-interval", 0, Enumeration({
        {u"1/128", t2_gi_1_128},
        {u"1/32", t2_gi_1_32},
        {u"1/16", t2_gi_1_16},
        {u"19/256", t2_gi_19_256},
        {u"1/8", t2_gi_1_8},
        {u"19/128", t2_gi_19_128},
        {u"1/4", t2_gi_1_4},
    }));
    help(u"t2-guard-interval",
         u"DVB-T2: indicates the guard interval. The default is 1/128.");

    option(u"t2-l1-modulation", 0, Enumeration({
        {u"BPSK",   t2_l1_bpsk},
        {u"QPSK",   t2_l1_qpsk},
        {u"16-QAM", t2_l1_qam16},
        {u"64-QAM", t2_l1_qam64},
    }));
    help(u"t2-l1-modulation",
         u"DVB-T2: indicate the modulation type used for the L1-post "
         u"signalling block. The default is 16-QAM.");

    option(u"t2-network-id", 0, UINT32);
    help(u"t2-network-id",
         u"DVB-T2: indicate the DVB-T2 network identification. "
         u"The default is 0.");

    option(u"t2-system-id", 0, UINT32);
    help(u"t2-system-id",
         u"DVB-T2: indicate the DVB-T2 system identification. "
         u"The default is 0.");

    option(u"bandwidth-extension");
    help(u"bandwidth-extension",
         u"DVB-T2: indicate that the extended carrier mode is used. "
         u"By default, use normal carrier mode.");

    option(u"transmission-mode", 't', Enumeration({
        {u"2K", fft_2k},
        {u"4K", fft_4k},
        {u"8K", fft_8k},
    }));
    help(u"transmission-mode",
         u"DVB-T, ISDB-T: indicate the transmission mode. The default is 8K.");

    option(u"modulation", 'm', Enumeration({
        {u"DVB-T",    modulator_dvb_t},
        {u"DVB-T2",   modulator_dvb_t2},
        {u"J83A",     modulator_j83a},
        {u"ATSC-VSB", modulator_atsc},
        {u"J83B",     modulator_j83b},
        {u"ISDB-T",   modulator_isdb_t},
        {u"J83C",     modulator_j83c},
        {u"DMB-T",    modulator_dtmb},
        {u"DTMB",     modulator_dtmb},
    }));
    help(u"modulation",
         u"Indicate the modulation type. "
         u"The supported modulation types depend on the device model. "
         u"The default modulation type is DVB-T.");

    option(u"remux", 0, Enumeration({
        {u"remux",       ustream_remux_pcr},
        {u"passthrough", ustream_remux_passthrough},
    }));
    help(u"remux",
         u"remux: Lock the first PCR to keep USB transfer TS stable, TS must contain some PCR to operate.\n"
         u"passthrough: Bypass TS without padding null packets (input bitrate = output bitrate).");

    option(u"pcradjust", 0, Enumeration({
        {u"disable", pcr_disable},
        {u"adjust",  pcr_adjust},
    }));
    help(u"pcradjust", u"Adjust the buffer transmission speed according to different application.");
}


//----------------------------------------------------------------------------
// Output plugin destructor
//----------------------------------------------------------------------------

ts::VatekOutputPlugin::~VatekOutputPlugin()
{
    if (_guts != nullptr) {
        VatekOutputPlugin::stop();
        delete _guts;
        _guts = nullptr;
    }
}


//----------------------------------------------------------------------------
// Output start method
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::start()
{
    return _guts->start();
}

bool ts::VatekOutputPlugin::Guts::start()
{
    vatek_result nres = vatek_badstatus;
    if (m_hdevices) {
        tsp->error(u"bad status already broadcasting.");
    }
    else {
        nres = configParam();
        if (is_vatek_success(nres)) {
            nres = vatek_device_list_enum(DEVICE_BUS_USB, service_transform, &m_hdevices);
            if (is_vatek_success(nres)) {
                if (nres == vatek_success) {
                    nres = vatek_nodevice;
                }
                else if (nres > vatek_success) {
                    nres = vatek_device_open(m_hdevices, m_index, &m_hchip);
                    if (is_vatek_success(nres)) {
                        nres = vatek_usbstream_open(m_hchip, &m_husbstream);
                    }
                    if (!is_vatek_success(nres)) {
                        tsp->error(u"open modulation device fail : [%d:%d]", { m_index,nres });
                    }
                    else {
                        nres = vatek_usbstream_start(m_husbstream, &m_param);
                        if (is_vatek_success(nres)) {
                            TS_PUSH_WARNING()
                            TS_LLVM_NOWARNING(cast-qual)
                            tsp->info(u"modulation start - [%s:%s:%d]", {vatek_device_get_name(m_hchip), ui_enum_get_str(modulator_type,m_param.modulator.type)});
                            TS_POP_WARNING()
                        }
                        else {
                            tsp->error(u"start modulation device broadcasting fail : [%d]", { nres });
                        }
                    }
                }
                if (!is_vatek_success(nres)) {
                    stop();
                }
            }
            else {
                tsp->error(u"enumeration modulation device fail : [%d]", { nres });
            }
        }
    }

    return is_vatek_success(nres);
}


//----------------------------------------------------------------------------
// Output stop method
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::stop()
{
    return _guts->stop();
}

bool ts::VatekOutputPlugin::Guts::stop()
{
    if (m_husbstream) {
        usbstream_status status = vatek_usbstream_get_status(m_husbstream, nullptr);
        if (status == usbstream_status_running || status == usbstream_status_moredata) {
            vatek_usbstream_stop(m_husbstream);
        }
    }
    if (m_husbstream) {
        vatek_usbstream_close(m_husbstream);
    }
    if (m_hchip) {
        vatek_device_close(m_hchip);
    }
    if (m_hdevices) {
        vatek_device_list_free(m_hdevices);
    }
    m_hdevices = nullptr;
    m_hchip = nullptr;
    m_husbstream = nullptr;
    modulator_param_reset(modulator_dvb_t, &m_param.modulator);
    m_index = -1;
    m_param.r2param.freqkhz = 473000;
    return true;
}


//----------------------------------------------------------------------------
// Output method
//----------------------------------------------------------------------------

bool ts::VatekOutputPlugin::send(const TSPacket* pkts, const TSPacketMetadata* meta, size_t packet_count)
{
    return _guts->send(pkts, meta, packet_count);
}

bool ts::VatekOutputPlugin::Guts::send(const TSPacket* pkts, const TSPacketMetadata* meta, size_t packet_count)
{
    vatek_result nres = vatek_badstatus;
    usbstream_status status = vatek_usbstream_get_status(m_husbstream, nullptr);

    if (status == usbstream_status_running || status == usbstream_status_moredata) {
        char* data = reinterpret_cast<char*>(const_cast<TSPacket*>(pkts));

        size_t remain = packet_count;
        while (remain > 0) {
            if (!m_slicebuf) {
                nres = vatek_ustream_async_get_buffer(m_husbstream, &m_slicebuf);
            }
            else {
                nres = vatek_result(1);
            }
            if (nres > vatek_success) {
                size_t pktnums = m_slicebuf->packet_len - m_slicebuf->packet_pos;
                if (pktnums > remain) {
                    pktnums = remain;
                }
                size_t pktsize = pktnums * TS_PACKET_LEN;
                memcpy(m_slicebuf->ptrbuf, data, pktsize);
                m_slicebuf->ptrbuf += pktsize;
                data += pktsize;
                m_slicebuf->packet_pos += int32_t(pktnums);
                if (m_slicebuf->packet_pos == m_slicebuf->packet_len) {
                    nres = vatek_ustream_async_commit_buffer(m_husbstream, m_slicebuf);
                    m_slicebuf = nullptr;
                }
                remain -= pktnums;
            }
            else if (nres == vatek_success) {
                cross_os_sleep(1);
            }
            if (!is_vatek_success(nres)){
                break;
            }
        }
    }

    if (is_vatek_success(nres)) {
        return true;
    }
    else {
        tsp->error(u"send packets to modulation fail : [%d]", { nres });
        return false;
    }
}


//----------------------------------------------------------------------------
// Get output bitrate
//----------------------------------------------------------------------------

ts::BitRate ts::VatekOutputPlugin::getBitrate()
{
    if (_guts->m_husbstream) {
        const uint32_t bitrate = modulator_param_get_bitrate(&_guts->m_param.modulator);
        tsp->debug(u"BitRate : [%d]", {bitrate});
        return BitRate(bitrate);
    }
    else  {
        return 0;
    }
}

ts::BitRateConfidence ts::VatekOutputPlugin::getBitrateConfidence()
{
    // The returned bitrate is based on the Vatek device hardware.
    return BitRateConfidence::HARDWARE;
}


//----------------------------------------------------------------------------
// Get modulation parameters from command line.
//----------------------------------------------------------------------------

vatek_result ts::VatekOutputPlugin::Guts::configParam()
{
#define MOD_NUMS (sizeof(m_modtables)/sizeof(tsmod_param_config))

    vatek_result nres = vatek_badparam;
    modulator_type type = modulator_type(plugin->intValue(u"modulation", modulator_dvb_t));

    for (size_t i = 0; i < MOD_NUMS; i++) {
        if (m_modtables[i].type == type) {
            nres = (this->*(m_modtables[i].config))(&m_param.modulator);
            break;
        }
    }

    if (is_vatek_success(nres)) {
        // Command line parameter is in Hz, Vatek parameter is in kHz.
        m_param.r2param.freqkhz = uint32_t(plugin->intValue<uint64_t>(u"frequency", uint64_t(m_param.r2param.freqkhz) * 1000) / 1000);
        plugin->getIntValue(m_index, u"device", 0);
        plugin->getIntValue(m_param.remux, u"remux", m_param.remux);
        plugin->getIntValue(m_param.pcradjust, u"pcradjust", m_param.pcradjust);
        debugParams();
    }

    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_dvb_t(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_dvb_t, pmod);
    if (is_vatek_success(nres)) {
        uint32_t val = plugin->intValue<uint32_t>(u"bandwidth", 8);
        if (val == 0 || val == 10) {
            nres = vatek_badparam;
            tsp->error(u"dvb-t not support bandwidth : %d", { val } );
        }
        else {
            Pdvb_t_param pdvb = &pmod->mod.dvb_t;
            pmod->bandwidth_symbolrate = val;
            plugin->getIntValue(pdvb->constellation, u"constellation", pdvb->constellation);
            plugin->getIntValue(pdvb->fft, u"transmission-mode", pdvb->fft);
            plugin->getIntValue(pdvb->coderate, u"convolutional-rate", pdvb->coderate);
            plugin->getIntValue(pdvb->guardinterval, u"guard-interval", pdvb->guardinterval);
            nres = modulator_param_get_bitrate(pmod) == 0 ? vatek_badparam : vatek_success;
            if (!is_vatek_success(nres)) {
                tsp->error(u"dvb-t param config fail : [%d]", {nres});
            }
        }
    }
    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_j83a(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_j83a, pmod);
    if (is_vatek_success(nres)) {
        uint32_t val = plugin->intValue<uint32_t>(u"symbol-rate", 5120000) / 1000;
        nres = vatek_badparam;
        if (val < 5000 || val > 7000) {
            tsp->error(u"j83a symbol-rate must between 5000 and 7000 ksym/s : [%d]", { val });
        }
        else {
            Pj83a_param pj83a = &pmod->mod.j83a;
            pmod->bandwidth_symbolrate = val;
            plugin->getIntValue(pj83a->constellation, u"j83-qam", j83a_qam128);
            nres = modulator_param_get_bitrate(pmod) == 0 ? vatek_badparam : vatek_success;
            if (!is_vatek_success(nres)) {
                tsp->error(u"j83a param config fail : [%d]", {nres});
            }
        }
    }
    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_j83c(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_j83c, pmod);
    if (is_vatek_success(nres)) {
        Pj83c_param pj83c = &pmod->mod.j83c;
        plugin->getIntValue(pj83c->constellation, u"j83-qam", j83a_qam256);
        if (pj83c->constellation == j83a_qam64) {
            pj83c->constellation = j83c_qam64;
        }
        else if (pj83c->constellation == j83a_qam256) {
            pj83c->constellation = j83c_qam256;
        }
        else {
            tsp->error(u"j83c only supports 64-QAM and 256-QAM");
            nres = vatek_badparam;
        }
    }
    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_atsc(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_atsc, pmod);
    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_j83b(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_j83b, pmod);
    if (is_vatek_success(nres)) {
        Pj83b_param pj83b = &pmod->mod.j83b;
        plugin->getIntValue(pj83b->constellation, u"j83-qam", j83a_qam256);
        if (pj83b->constellation == j83a_qam64) {
            pj83b->constellation = j83b_qam64;
        }
        else if (pj83b->constellation == j83a_qam256) {
            pj83b->constellation = j83b_qam256;
        }
        else {
            tsp->error(u"j83b only supports 64-QAM and 256-QAM");
            nres = vatek_badparam;
        }
    }
    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_dtmb(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_dtmb, pmod);
    if (is_vatek_success(nres)) {
        uint32_t val = plugin->intValue<uint32_t>(u"bandwidth", 8);
        nres = vatek_badparam;
        if (val == 0 || val == 10) {
            tsp->error(u"dtmb does not support bandwidth : %d", { val });
        }
        else {
            Pdtmb_param pdtmb = &pmod->mod.dtmb;
            pmod->bandwidth_symbolrate = val;
            plugin->getIntValue(pdtmb->constellation, u"dmb-constellation", pdtmb->constellation);
            plugin->getIntValue(pdtmb->coderate, u"dmb-fec", pdtmb->coderate);
            plugin->getIntValue(pdtmb->framesync, u"dmb-header", pdtmb->framesync);
            plugin->getIntValue(pdtmb->timeinterleaved, u"dmb-interleaver", pdtmb->timeinterleaved);
            plugin->getIntValue(pdtmb->carriermode, u"dmb-carrier", pdtmb->carriermode);

            if ((pdtmb->constellation == dtmb_qam4_nr || pdtmb->constellation == dtmb_qam32) && pdtmb->coderate != dtmb_code_rate_0_8) {
                tsp->error(u"dtmb qam4-nr and qam32 only support dmb-fec = 0.8.");
            }
            else {
                nres = modulator_param_get_bitrate(pmod) == 0 ? vatek_badparam : vatek_success;
                if (!is_vatek_success(nres)) {
                    tsp->error(u"dtmb param config fail : [%d]", {nres});
                }
            }
        }
    }
    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_isdb_t(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_isdb_t, pmod);
    if (is_vatek_success(nres)) {
        uint32_t val = plugin->intValue<uint32_t>(u"bandwidth", 6);
        nres = vatek_badparam;
        if (val == 0 || val == 10) {
            tsp->error(u"isdb-t does not support bandwidth : %d", { val });
        }
        else {
            Pisdb_t_param pisdbt = &pmod->mod.isdb_t;
            pmod->bandwidth_symbolrate = val;
            plugin->getIntValue(pisdbt->constellation, u"constellation", dvb_t_qam64);
            if (pisdbt->constellation == dvb_t_qpsk) {
                pisdbt->constellation = isdb_t_qpsk;
            }
            else if (pisdbt->constellation == dvb_t_qam16) {
                pisdbt->constellation = isdb_t_qam16;
            }
            else {
                pisdbt->constellation = isdb_t_qam64;
            }
            plugin->getIntValue(pisdbt->fft, u"transmission-mode", pisdbt->fft);
            plugin->getIntValue(pisdbt->coderate, u"convolutional-rate", pisdbt->coderate);
            plugin->getIntValue(pisdbt->guardinterval, u"guard-interval", pisdbt->guardinterval);

            if (pisdbt->fft == fft_8k) {
                pisdbt->timeinterleaved = isdb_t_interleaved_mode3;
            }
            else if (pisdbt->fft == fft_4k) {
                pisdbt->timeinterleaved = isdb_t_interleaved_mode2;
            }
            else {
                pisdbt->timeinterleaved = isdb_t_interleaved_mode1;
            }
            nres = modulator_param_get_bitrate(pmod) == 0 ? vatek_badparam : vatek_success;
            if (!is_vatek_success(nres)) {
                tsp->error(u"isdb-t param config fail : [%d]", {nres});
            }
        }
    }
    return nres;
}

vatek_result ts::VatekOutputPlugin::Guts::modparam_config_dvb_t2(Pmodulator_param pmod)
{
    vatek_result nres = modulator_param_reset(modulator_dvb_t2, pmod);
    if (is_vatek_success(nres))  {
        uint32_t val = plugin->intValue<uint32_t>(u"bandwidth", 8);
        Pdvb_t2_param pt2 = &pmod->mod.dvb_t2;
        pmod->bandwidth_symbolrate = val;
        plugin->getIntValue(pt2->version, u"t2-version", pt2->version);
        plugin->getIntValue(pt2->l1_constellation, u"t2-l1-modulation", pt2->l1_constellation);
        plugin->getIntValue(pt2->coderate, u"plp0-code-rate", pt2->coderate);
        plugin->getIntValue(pt2->fectype, u"plp0-fec-type", pt2->fectype);
        plugin->getIntValue(pt2->plp_constellation, u"plp0-modulation", pt2->plp_constellation);
        plugin->getIntValue(pt2->fft, u"fft-mode", pt2->fft);
        plugin->getIntValue(pt2->guardinterval, u"t2-guard-interval", pt2->guardinterval);
        plugin->getIntValue(pt2->pilotpattern, u"pilot-pattern", pt2->pilotpattern);
        plugin->getIntValue(pt2->network_id, u"t2-network-id", pt2->network_id);
        plugin->getIntValue(pt2->system_id, u"t2-system-id", pt2->system_id);
        plugin->getIntValue(pt2->issy, u"plp0-issy", pt2->issy);

        pt2->t2_flags = 0;
        if (plugin->present(u"bandwidth-extension")) {
            pt2->t2_flags |= T2EN_EXTEND_CARRIER_MODE;
        }
        if (plugin->present(u"plp0-rotation")) {
            pt2->t2_flags |= T2EN_CONSTELLATION_ROTATION;
        }
        if (plugin->present(u"plp0-null-packet-deletion")) {
            pt2->t2_flags |= T2EN_DELETE_NULL_PACKET;
        }
        if (plugin->present(u"plp0-high-efficiency")) {
            pt2->t2_flags |= T2EN_INPUT_TS_HEM;
        }
        pt2->fecblock_nums = 0;
        pt2->symbol_nums = 0;

        nres = modulator_param_get_bitrate(pmod) == 0 ? vatek_badparam : vatek_success;
        if (!is_vatek_success(nres)) {
            tsp->error(u"dvb-t2 param config fail : [%d]", {nres});
        }
    }
    return nres;
}


//----------------------------------------------------------------------------
// Display modulation parameters in debug mode.
//----------------------------------------------------------------------------

void ts::VatekOutputPlugin::Guts::debugParams()
{
    if (tsp->debug()) {
        #define PR(format,name) tsp->debug(u"" #name " = " format, {m_param.name})
        PR("%d", mode);
        PR("%d", remux);
        PR("%d", pcradjust);
        PR("%d", r2param.mode);
        PR("0x%X", r2param.r2_flags);
        PR("%'d", r2param.freqkhz);
        PR("%d", r2param.rule.tune.ioffset);
        PR("%d", r2param.rule.tune.qoffset);
        PR("%d", r2param.rule.tune.imgoffset);
        PR("%d", r2param.rule.tune.phaseoffset);
        PR("%d", r2param.rule.pagain);
        PR("%d", r2param.rule.gpiocntl);
        PR("%d", modulator.bandwidth_symbolrate);
        PR("%d", modulator.type);
        PR("%d", modulator.ifmode);
        PR("%d", modulator.iffreq_offset);
        PR("%d", modulator.dac_gain);
        PR("%d", modulator.mod.raw_byte);
        switch (m_param.modulator.type) {
            case modulator_dvb_t:
                PR("%d", modulator.mod.dvb_t.constellation);
                PR("%d", modulator.mod.dvb_t.fft);
                PR("%d", modulator.mod.dvb_t.guardinterval);
                PR("%d", modulator.mod.dvb_t.coderate);
                break;
            case modulator_dvb_t2:
                PR("%d", modulator.mod.dvb_t2.version);
                PR("0x%X", modulator.mod.dvb_t2.t2_flags);
                PR("%d", modulator.mod.dvb_t2.l1_constellation);
                PR("%d", modulator.mod.dvb_t2.plp_constellation);
                PR("%d", modulator.mod.dvb_t2.issy);
                PR("%d", modulator.mod.dvb_t2.fft);
                PR("%d", modulator.mod.dvb_t2.coderate);
                PR("%d", modulator.mod.dvb_t2.guardinterval);
                PR("%d", modulator.mod.dvb_t2.pilotpattern);
                PR("%d", modulator.mod.dvb_t2.fectype);
                PR("%d", modulator.mod.dvb_t2.network_id);
                PR("%d", modulator.mod.dvb_t2.system_id);
                PR("%d", modulator.mod.dvb_t2.fecblock_nums);
                PR("%d", modulator.mod.dvb_t2.symbol_nums);
                PR("%d", modulator.mod.dvb_t2.ti_ni);
                PR("%d", modulator.mod.dvb_t2.recv);
                break;
            case modulator_isdb_t:
                PR("%d", modulator.mod.isdb_t.constellation);
                PR("%d", modulator.mod.isdb_t.fft);
                PR("%d", modulator.mod.isdb_t.guardinterval);
                PR("%d", modulator.mod.isdb_t.coderate);
                PR("%d", modulator.mod.isdb_t.timeinterleaved);
                PR("0x%X", modulator.mod.isdb_t.isdb_t_flags);
                break;
            case modulator_atsc:
                PR("%d", modulator.mod.atsc.constellation);
                break;
            case modulator_j83a:
                PR("%d", modulator.mod.j83a.constellation);
                break;
            case modulator_j83b:
                PR("%d", modulator.mod.j83b.constellation);
                PR("%d", modulator.mod.j83b.r2_path);
                break;
            case modulator_j83c:
                PR("%d", modulator.mod.j83c.constellation);
                break;
            case modulator_dtmb:
                PR("%d", modulator.mod.dtmb.constellation);
                PR("%d", modulator.mod.dtmb.timeinterleaved);
                PR("%d", modulator.mod.dtmb.coderate);
                PR("%d", modulator.mod.dtmb.carriermode);
                PR("%d", modulator.mod.dtmb.framesync);
                break;
            case modulator_unknown:
            case modulator_mod_nums:
            default:
                break;
        }
        if (m_param.mode == ustream_mode_async) {
            PR("%d", async.mode);
            PR("%d", async.bitrate);
            PR("%d", async.prepare_ms);
        }
        #undef PR
    }
}

#endif // TS_NO_VATEK
