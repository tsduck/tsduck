//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//  Generic / sample / reference descrambler.
//  Can be used as a template for real conditional access systems.
//
//----------------------------------------------------------------------------

#include "tsAbstractDescrambler.h"
#include "tsPluginRepository.h"
#include "tsDuckProtocol.h"


//----------------------------------------------------------------------------
// Plugin definition
//----------------------------------------------------------------------------

namespace ts {
    class DescramblerPlugin: public AbstractDescrambler
    {
        TS_NOBUILD_NOCOPY(DescramblerPlugin);
    public:
        // Constructor.
        DescramblerPlugin(TSP*);

        // Implementation of ProcessorPlugin interface.
        virtual bool getOptions() override;

    protected:
        // Implementation of AbstractDescrambler.
        virtual bool checkCADescriptor(uint16_t pmt_cas_id, const ByteBlock& priv) override;
        virtual bool checkECM(const Section& ecm) override;
        virtual bool decipherECM(const Section& ecm, CWData& cw_even, CWData& cw_odd) override;

    private:
        // Private fields.
        uint16_t _cas_id;
    };
}

TS_REGISTER_PROCESSOR_PLUGIN(u"descrambler", ts::DescramblerPlugin);


//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------

ts::DescramblerPlugin::DescramblerPlugin(TSP* tsp_) :
    AbstractDescrambler(tsp_, u"Generic DVB descrambler"),
    _cas_id(0)
{
    option(u"cas-id", 0, UINT16);
    help(u"cas-id",
         u"Specify the CA_system_id to filter when searching for ECM streams. Since "
         u"this descrambler is a demo tool using clear ECM's, it is unlikely that "
         u"other real ECM streams exist. So, by default, any ECM stream is used to "
         u"get the clear ECM's.\n\n"
         u"This plugin descrambles fixed PID's with fixed control words. As a demo, it can "
         u"also descramble services for which clear ECM's were generated using the utility "
         u"named tsecmg, a DVB SimulCrypt-compliant ECMG for test and demo.");
}


//----------------------------------------------------------------------------
// Plugin get options method.
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::getOptions()
{
    // Load plugin-specific command line arguments.
    _cas_id = intValue<uint16_t>(u"cas-id", 0);

    // The invoke superclass to actually start the descrambler.
    return AbstractDescrambler::getOptions();
}


//----------------------------------------------------------------------------
// Check a CA_descriptor from a PMT.
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::checkCADescriptor(uint16_t pmt_cas_id, const ByteBlock& priv)
{
    // In this demo descrambler, we accept all CAS id, unless one is specified.
    return _cas_id == 0 || pmt_cas_id == _cas_id;
}


//----------------------------------------------------------------------------
// Check if the descrambler may decipher an ECM.
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::checkECM(const Section& ecm)
{
    // In this demo descrambler, we do not add any further check.
    return true;
}


//----------------------------------------------------------------------------
// Decipher an ECM.
//----------------------------------------------------------------------------

bool ts::DescramblerPlugin::decipherECM(const Section& ecm, CWData& cw_even, CWData& cw_odd)
{
    // Clear returned ECM's.
    cw_even.cw.clear();
    cw_even.iv.clear();
    cw_odd.cw.clear();
    cw_odd.iv.clear();

    // The ECM content is the section payload.
    const uint8_t* const ecmData = ecm.payload();
    const size_t ecmSize = ecm.payloadSize();

    // Analyze ECM as a TLV message.
    tlv::MessageFactory mf(ecmData, ecmSize, duck::Protocol::Instance());
    tlv::MessagePtr msg(mf.factory());

    // If this a valid ECM?
    SafePtr<duck::ClearECM> clearECM(msg.downcast<duck::ClearECM>());
    if (clearECM.isNull()) {
        // Not a valid ECM as generated by tsecmg.
        const size_t dumpSize = std::min<size_t>(ecmSize, 16);
        tsp->error(u"received invalid ECM (%d bytes): %s%s", {ecmSize, UString::Dump(ecmData, dumpSize, UString::SINGLE_LINE), dumpSize < ecmSize ? u" ..." : u""});
        return false;
    }
    else {
        cw_even.cw = clearECM->cw_even;
        cw_odd.cw = clearECM->cw_odd;
        tsp->verbose(u"ECM found, even CW: %s, odd CW: %s", {UString::Dump(cw_even.cw, UString::COMPACT), UString::Dump(cw_odd.cw, UString::COMPACT)});
        return true;
    }
}
