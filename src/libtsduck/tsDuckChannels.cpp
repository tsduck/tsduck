//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2019, Thierry Lelegard
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

#include "tsDuckChannels.h"
#include "tsModulation.h"
#include "tsxmlElement.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::DuckChannels::DuckChannels() :
    networks(),
    _xmlTweaks()
{
}

ts::DuckChannels::Service::Service() :
    id(0),
    name(),
    provider(),
    lcn(),
    pmtPID(),
    type(),
    cas()
{
}

ts::DuckChannels::TransportStream::TransportStream() :
    id(0),
    onid(0),
    tune(),
    services()
{
}

ts::DuckChannels::Network::Network() :
    id(0),
    type(DVB_S),
    ts()
{
}


//----------------------------------------------------------------------------
// Clear the loaded content.
//----------------------------------------------------------------------------

void ts::DuckChannels::clear()
{
    networks.clear();
}


//----------------------------------------------------------------------------
// Default XML channel file name.
//----------------------------------------------------------------------------

ts::UString ts::DuckChannels::DefaultFileName()
{
#if defined(TS_WINDOWS)
    static const UChar env[] = u"APPDATA";
    static const UChar name[] = u"\\tsduck\\channels.xml";
#else
    static const UChar env[] = u"HOME";
    static const UChar name[] = u"/.tsduck.channels.xml";
#endif

    const UString root(GetEnvironment(env));
    return root.empty() ? UString() : UString(root) + UString(name);
}


//----------------------------------------------------------------------------
// Load / parse an XML file.
//----------------------------------------------------------------------------

bool ts::DuckChannels::load(const UString& fileName, Report& report)
{
    clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(fileName, false) && parseDocument(doc);
}

bool ts::DuckChannels::load(std::istream& strm, Report& report)
{
    clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(strm) && parseDocument(doc);
}

bool ts::DuckChannels::parse(const UString& text, Report& report)
{
    clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.parse(text) && parseDocument(doc);
}

bool ts::DuckChannels::parseDocument(const xml::Document& doc)
{
    // Load the XML model for TSDuck files. Search it in TSDuck directory.
    xml::Document model(doc.report());
    if (!model.load(u"tsduck.channels.xml", true)) {
        doc.report().error(u"Model for TSDuck channels XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!doc.validate(model)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();
    bool success = true;

    // Analyze all networks in the document.
    for (const xml::Element* node = root == nullptr ? nullptr : root->firstChildElement(); node != nullptr; node = node->nextSiblingElement()) {
        // @@@@@
    }
    return success;
}


//----------------------------------------------------------------------------
// Create XML file or text.
//----------------------------------------------------------------------------

bool ts::DuckChannels::save(const UString& fileName, Report& report) const
{
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) && doc.save(fileName);
}

ts::UString ts::DuckChannels::toXML(Report& report) const
{
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) ? doc.toString() : UString();
}


//----------------------------------------------------------------------------
// Generate an XML document.
//----------------------------------------------------------------------------

bool ts::DuckChannels::generateDocument(xml::Document& doc) const
{
    // Initialize the document structure.
    xml::Element* root = doc.initialize(u"tsduck");
    if (root == nullptr) {
        return false;
    }

    // Format all networks.
    for (auto itnet = networks.begin(); itnet != networks.end(); ++itnet) {
        const NetworkPtr& net(*itnet);
        if (!net.isNull()) {

            // Create one network element.
            xml::Element* xnet = root->addElement(u"network");
            xnet->setIntAttribute(u"id", net->id, true);
            xnet->setEnumAttribute(TunerTypeEnum, u"type", net->type);

            // Format all transport streams.
            for (auto itts = net->ts.begin(); itts != net->ts.end(); ++itts) {
                const TransportStreamPtr& ts(*itts);
                if (!ts.isNull()) {

                    // Create one transport stream element.
                    xml::Element* xts = xnet->addElement(u"ts");
                    xts->setIntAttribute(u"id", ts->id, true);
                    xts->setIntAttribute(u"onid", ts->onid, true);

                    // Set tuner parameters. Try various options in sequence.
                    TS_UNUSED const bool done = 
                        TunerToXML(xts, dynamic_cast<const TunerParametersDVBT*>(ts->tune.pointer())) != nullptr ||
                        TunerToXML(xts, dynamic_cast<const TunerParametersDVBS*>(ts->tune.pointer())) != nullptr ||
                        TunerToXML(xts, dynamic_cast<const TunerParametersDVBC*>(ts->tune.pointer())) != nullptr ||
                        TunerToXML(xts, dynamic_cast<const TunerParametersATSC*>(ts->tune.pointer())) != nullptr;

                    // Format all services.
                    for (auto itsrv = ts->services.begin(); itsrv != ts->services.end(); ++itsrv) {
                        const ServicePtr& srv(*itsrv);
                        if (!srv.isNull()) {

                            // Create one service element.
                            xml::Element* xsrv = xts->addElement(u"service");
                            xsrv->setIntAttribute(u"id", srv->id, true);
                            xsrv->setAttribute(u"name", srv->name, true);
                            xsrv->setAttribute(u"provider", srv->provider, true);
                            xsrv->setOptionalIntAttribute(u"LCN", srv->lcn, true);
                            xsrv->setOptionalIntAttribute(u"PMTPID", srv->pmtPID, true);
                            xsrv->setOptionalIntAttribute(u"type", srv->type, true);
                            xsrv->setOptionalBoolAttribute(u"cas", srv->cas);
                        }
                    }
                }
            }
        }
    }
    return true;
}

//----------------------------------------------------------------------------
// Generate an XML element from a set of tuner parameters.
//----------------------------------------------------------------------------

ts::xml::Element* ts::DuckChannels::TunerToXML(xml::Element* parent, const TunerParametersATSC* params)
{
    if (params == nullptr || parent == nullptr) {
        return nullptr;
    }
    else {
        xml::Element* e = parent->addElement(u"atsc");
        e->setIntAttribute(u"frequency", params->frequency, false);
        e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
        return e;
    }
}

ts::xml::Element* ts::DuckChannels::TunerToXML(xml::Element* parent, const TunerParametersDVBC* params)
{
    if (params == nullptr || parent == nullptr) {
        return nullptr;
    }
    else {
        xml::Element* e = parent->addElement(u"dvbc");
        e->setIntAttribute(u"frequency", params->frequency, false);
        e->setIntAttribute(u"symbolrate", params->symbol_rate, false);
        e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        e->setEnumAttribute(InnerFECEnum, u"FEC", params->inner_fec);
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
        return e;
    }
}

ts::xml::Element* ts::DuckChannels::TunerToXML(xml::Element* parent, const TunerParametersDVBS* params)
{
    if (params == nullptr || parent == nullptr) {
        return nullptr;
    }
    else {
        xml::Element* e = parent->addElement(u"dvbs");
        if (params->satellite_number != 0) {
            e->setIntAttribute(u"satellite", params->satellite_number, false);
        }
        e->setIntAttribute(u"frequency", params->frequency, false);
        if (params->polarity != POL_AUTO) {
            e->setEnumAttribute(PolarizationEnum, u"polarity", params->polarity);
        }
        e->setIntAttribute(u"symbolrate", params->symbol_rate, false);
        e->setEnumAttribute(InnerFECEnum, u"FEC", params->inner_fec);
        e->setEnumAttribute(DeliverySystemEnum, u"system", params->delivery_system);
        if (params->delivery_system != DS_DVB_S) {
            e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        }
        if (params->delivery_system == DS_DVB_S2 && params->pilots != PILOT_AUTO) {
            e->setEnumAttribute(PilotEnum, u"pilots", params->pilots);
        }
        if (params->delivery_system == DS_DVB_S2 && params->roll_off != ROLLOFF_AUTO) {
            e->setEnumAttribute(RollOffEnum, u"rolloff", params->roll_off);
        }
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
        return e;
    }
}

ts::xml::Element* ts::DuckChannels::TunerToXML(xml::Element* parent, const TunerParametersDVBT* params)
{
    if (params == nullptr || parent == nullptr) {
        return nullptr;
    }
    else {
        xml::Element* e = parent->addElement(u"dvbt");
        e->setIntAttribute(u"frequency", params->frequency, false);
        e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        /*@@@@@
        modulation="QPSK|16-QAM|64-QAM|256-QAM, default=64-QAM"
        HPFEC="1/2|2/3|3/4|4/5|5/6|6/7|7/8|8/9|9/10|3/5|1/3|1/4|2/5|5/11|auto|none, default=auto"
        LPFEC="1/2|2/3|3/4|4/5|5/6|6/7|7/8|8/9|9/10|3/5|1/3|1/4|2/5|5/11|auto|none, default=auto"
        bandwidth="1.712-MHz|5-MHz|6-MHz|7-MHz|8-MHz|10-MHz|auto, default=auto"
        transmission="1K|2K|4K|8K|16K|32K|auto, default=auto"
        guard="1/4|1/8|1/16|1/32|1/128|19/128|19/256|auto, default=auto"
        hierarchy="1|2|4|auto|none, default=none"

        BandWidth         bandwidth;          //!< Bandwidth.
        InnerFEC          fec_hp;             //!< High priority stream code rate.
        InnerFEC          fec_lp;             //!< Low priority stream code rate.
        Modulation        modulation;         //!< Constellation (modulation type).
        TransmissionMode  transmission_mode;  //!< Transmission mode.
        GuardInterval     guard_interval;     //!< Guard interval.
        Hierarchy         hierarchy;          //!< Hierarchy.
        */
        if (params->plp != PLP_DISABLE) {
            e->setIntAttribute(u"PLP", uint8_t(params->plp), false);
        }
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
        return e;
    }
}
