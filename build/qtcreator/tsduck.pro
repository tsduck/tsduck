#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2018, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
# Top-level qmake project file for TSDuck.
#
#-----------------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    libtsduck \
    tsplugin_aes \
    tsplugin_analyze \
    tsplugin_bat \
    tsplugin_bitrate_monitor \
    tsplugin_boostpid \
    tsplugin_cat \
    tsplugin_clear \
    tsplugin_continuity \
    tsplugin_count \
    tsplugin_datainject \
    tsplugin_decap \
    tsplugin_dektec \
    tsplugin_descrambler \
    tsplugin_drop \
    tsplugin_duplicate \
    tsplugin_dvb \
    tsplugin_eit \
    tsplugin_encap \
    tsplugin_file \
    tsplugin_filter \
    tsplugin_fork \
    tsplugin_hides \
    tsplugin_history \
    tsplugin_hls \
    tsplugin_http \
    tsplugin_inject \
    tsplugin_ip \
    tsplugin_limit \
    tsplugin_merge \
    tsplugin_mpe  \
    tsplugin_mpeinject \
    tsplugin_mux \
    tsplugin_nit \
    tsplugin_nitscan \
    tsplugin_null \
    tsplugin_pat \
    tsplugin_pattern \
    tsplugin_pcrbitrate \
    tsplugin_pcrextract \
    tsplugin_pcrverify \
    tsplugin_pes \
    tsplugin_play \
    tsplugin_pmt \
    tsplugin_psi \
    tsplugin_reduce \
    tsplugin_regulate \
    tsplugin_remap \
    tsplugin_rmorphan \
    tsplugin_rmsplice \
    tsplugin_scrambler \
    tsplugin_sdt \
    tsplugin_sections \
    tsplugin_sifilter \
    tsplugin_skip \
    tsplugin_slice \
    tsplugin_spliceinject \
    tsplugin_stuffanalyze \
    tsplugin_svremove \
    tsplugin_svrename \
    tsplugin_t2mi \
    tsplugin_tables \
    tsplugin_teletext \
    tsplugin_time \
    tsplugin_timeref \
    tsplugin_tsrename \
    tsplugin_until \
    tsplugin_zap \
    utest \
    tsanalyze \
    tsbitrate \
    tscmp \
    tsdate \
    tsdektec \
    tsdump \
    tsecmg \
    tsemmg \
    tsfixcc \
    tsftrunc \
    tsgenecm \
    tshides \
    tslsdvb \
    tsp \
    tspacketize \
    tspsi \
    tsresync \
    tsscan \
    tssmartcard \
    tsstuff \
    tsswitch \
    tstabcomp \
    tstabdump \
    tstables \
    tsterinfo \
    tsversion
