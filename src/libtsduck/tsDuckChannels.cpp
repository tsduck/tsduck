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
// Load an XML file or text.
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


//----------------------------------------------------------------------------
// Parse an XML document.
//----------------------------------------------------------------------------

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
    assert(root != nullptr);
    xml::ElementVector xnets;
    root->getChildren(xnets, u"network");
    for (auto itnet = xnets.begin(); itnet != xnets.end(); ++itnet) {
        
        // Build a new Network object at end of our list of networks.
        const NetworkPtr net(new Network);
        CheckNonNull(net.pointer());
        networks.push_back(net);

        // Get network properties.
        xml::ElementVector xts;
        success =
            (*itnet)->getIntAttribute<uint16_t>(net->id, u"id", true) &&
            (*itnet)->getIntEnumAttribute(net->type, TunerTypeEnum, u"type", true) &&
            (*itnet)->getChildren(xts, u"ts") &&
            success;

        // Get all TS in the network.
        for (auto itts = xts.begin(); itts != xts.end(); ++itts) {

            // Build a new TransportStream object at end of TS for the current network.
            const TransportStreamPtr ts(new TransportStream);
            CheckNonNull(ts.pointer());
            net->ts.push_back(ts);

            // Get transport stream properties.
            xml::ElementVector xservices;
            xml::ElementVector xatsc;
            xml::ElementVector xdvbc;
            xml::ElementVector xdvbs;
            xml::ElementVector xdvbt;
            success =
                (*itts)->getIntAttribute<uint16_t>(ts->id, u"id", true) &&
                (*itts)->getIntAttribute<uint16_t>(ts->onid, u"onid", true) &&
                (*itts)->getChildren(xatsc, u"atsc", 0, 1) &&
                (*itts)->getChildren(xdvbc, u"dvbc", 0, 1) &&
                (*itts)->getChildren(xdvbs, u"dvbs", 0, 1) &&
                (*itts)->getChildren(xdvbt, u"dvbt", 0, 1) &&
                (*itts)->getChildren(xservices, u"service") &&
                success;

            // Get tuner parameters (at most one structure is allowed).
            if (xatsc.size() + xdvbc.size() + xdvbs.size() + xdvbt.size() > 1) {
                doc.report().error(u"At most one of <atsc>, <dvbc>, <dvbs>, <dvbt> is allowed in <ts> at line %d", {(*itts)->lineNumber()});
                success = false;
            }
            else if (!xatsc.empty()) {
                success = XmlToATCS(ts->tune, xatsc.front()) && success;
            }
            else if (!xdvbc.empty()) {
                success = XmlToDVBC(ts->tune, xdvbc.front()) && success;
            }
            else if (!xdvbs.empty()) {
                success = XmlToDVBS(ts->tune, xdvbs.front()) && success;
            }
            else if (!xdvbt.empty()) {
                success = XmlToDVBT(ts->tune, xdvbt.front()) && success;
            }

            // Get all services in the transport stream.
            for (auto itsrv = xservices.begin(); itsrv != xservices.end(); ++itsrv) {

                // Build a new Service object at end of the current transport stream.
                const ServicePtr srv(new Service);
                CheckNonNull(srv.pointer());
                ts->services.push_back(srv);

                // Get service properties.
                success =
                    (*itsrv)->getIntAttribute<uint16_t>(srv->id, u"id", true) &&
                    (*itsrv)->getAttribute(srv->name, u"name", false) &&
                    (*itsrv)->getAttribute(srv->provider, u"provider", false) &&
                    (*itsrv)->getOptionalIntAttribute(srv->lcn, u"LCN") &&
                    (*itsrv)->getOptionalIntAttribute(srv->pmtPID, u"PMTPID", PID(0), PID(PID_NULL)) &&
                    (*itsrv)->getOptionalIntAttribute(srv->type, u"type") &&
                    (*itsrv)->getOptionalBoolAttribute(srv->cas, u"cas") &&
                    success;
            }
        }
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
                    // Typically, only one succeeds. No error if none works (this is just an incomplete description).
                    TunerToXml(xts, dynamic_cast<const TunerParametersDVBT*>(ts->tune.pointer()));
                    TunerToXml(xts, dynamic_cast<const TunerParametersDVBS*>(ts->tune.pointer()));
                    TunerToXml(xts, dynamic_cast<const TunerParametersDVBC*>(ts->tune.pointer()));
                    TunerToXml(xts, dynamic_cast<const TunerParametersATSC*>(ts->tune.pointer()));

                    // Format all services.
                    for (auto itsrv = ts->services.begin(); itsrv != ts->services.end(); ++itsrv) {
                        const ServicePtr& srv(*itsrv);
                        if (!srv.isNull()) {

                            // Create one service element.
                            xml::Element* xsrv = xts->addElement(u"service");
                            xsrv->setIntAttribute(u"id", srv->id, true);
                            xsrv->setAttribute(u"name", srv->name, true);
                            xsrv->setAttribute(u"provider", srv->provider, true);
                            xsrv->setOptionalIntAttribute(u"LCN", srv->lcn, false);
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

void ts::DuckChannels::TunerToXml(xml::Element* parent, const TunerParametersATSC* params)
{
    if (params != nullptr && parent != nullptr) {
        xml::Element* e = parent->addElement(u"atsc");
        e->setIntAttribute(u"frequency", params->frequency, false);
        e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
    }
}

void ts::DuckChannels::TunerToXml(xml::Element* parent, const TunerParametersDVBC* params)
{
    if (params != nullptr && parent != nullptr) {
        xml::Element* e = parent->addElement(u"dvbc");
        e->setIntAttribute(u"frequency", params->frequency, false);
        e->setIntAttribute(u"symbolrate", params->symbol_rate, false);
        e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        if (params->inner_fec != FEC_AUTO) {
            e->setEnumAttribute(InnerFECEnum, u"FEC", params->inner_fec);
        }
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
    }
}

void ts::DuckChannels::TunerToXml(xml::Element* parent, const TunerParametersDVBS* params)
{
    if (params != nullptr && parent != nullptr) {
        xml::Element* e = parent->addElement(u"dvbs");
        if (params->satellite_number != 0) {
            e->setIntAttribute(u"satellite", params->satellite_number, false);
        }
        e->setIntAttribute(u"frequency", params->frequency, false);
        e->setIntAttribute(u"symbolrate", params->symbol_rate, false);
        e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        if (params->delivery_system != DS_DVB_S) {
            e->setEnumAttribute(DeliverySystemEnum, u"system", params->delivery_system);
        }
        if (params->polarity != POL_AUTO) {
            e->setEnumAttribute(PolarizationEnum, u"polarity", params->polarity);
        }
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
        if (params->inner_fec != FEC_AUTO) {
            e->setEnumAttribute(InnerFECEnum, u"FEC", params->inner_fec);
        }
        if (params->delivery_system == DS_DVB_S2 && params->pilots != PILOT_AUTO) {
            e->setEnumAttribute(PilotEnum, u"pilots", params->pilots);
        }
        if (params->delivery_system == DS_DVB_S2 && params->roll_off != ROLLOFF_AUTO) {
            e->setEnumAttribute(RollOffEnum, u"rolloff", params->roll_off);
        }
    }
}

void ts::DuckChannels::TunerToXml(xml::Element* parent, const TunerParametersDVBT* params)
{
    if (params != nullptr && parent != nullptr) {
        xml::Element* e = parent->addElement(u"dvbt");
        e->setIntAttribute(u"frequency", params->frequency, false);
        e->setEnumAttribute(ModulationEnum, u"modulation", params->modulation);
        if (params->fec_hp != FEC_AUTO) {
            e->setEnumAttribute(InnerFECEnum, u"HPFEC", params->fec_hp);
        }
        if (params->fec_lp != FEC_AUTO) {
            e->setEnumAttribute(InnerFECEnum, u"LPFEC", params->fec_lp);
        }
        if (params->bandwidth != BW_AUTO) {
            e->setEnumAttribute(BandWidthEnum, u"bandwidth", params->bandwidth);
        }
        if (params->transmission_mode != TM_AUTO) {
            e->setEnumAttribute(TransmissionModeEnum, u"transmission", params->transmission_mode);
        }
        if (params->guard_interval != GUARD_AUTO) {
            e->setEnumAttribute(GuardIntervalEnum, u"guard", params->guard_interval);
        }
        if (params->hierarchy != HIERARCHY_NONE) {
            e->setEnumAttribute(HierarchyEnum, u"hierarchy", params->hierarchy);
        }
        if (params->plp != PLP_DISABLE) {
            e->setIntAttribute(u"PLP", uint8_t(params->plp), false);
        }
        if (params->inversion != SPINV_AUTO) {
            e->setEnumAttribute(SpectralInversionEnum, u"inversion", params->inversion);
        }
    }
}


//----------------------------------------------------------------------------
// Parse an XML element into a set of tuner parameters.
//----------------------------------------------------------------------------

bool ts::DuckChannels::XmlToATCS(TunerParametersPtr& params, const xml::Element* elem)
{
    TunerParametersATSC* p = new TunerParametersATSC;
    params = p;
    return p != nullptr && elem != nullptr &&
        elem->getIntAttribute<uint64_t>(p->frequency, u"frequency", true) &&
        elem->getIntEnumAttribute(p->modulation, ModulationEnum, u"modulation", false, VSB_8) &&
        elem->getIntEnumAttribute(p->inversion, SpectralInversionEnum, u"inversion", false, SPINV_AUTO);
}

bool ts::DuckChannels::XmlToDVBC(TunerParametersPtr& params, const xml::Element* elem)
{
    TunerParametersDVBC* p = new TunerParametersDVBC;
    params = p;
    return p != nullptr && elem != nullptr &&
        elem->getIntAttribute<uint64_t>(p->frequency, u"frequency", true) &&
        elem->getIntAttribute<uint32_t>(p->symbol_rate, u"symbolrate", false, 6900000) &&
        elem->getIntEnumAttribute(p->modulation, ModulationEnum, u"modulation", false, QAM_64) &&
        elem->getIntEnumAttribute(p->inner_fec, InnerFECEnum, u"FEC", false, FEC_AUTO) &&
        elem->getIntEnumAttribute(p->inversion, SpectralInversionEnum, u"inversion", false, SPINV_AUTO);
}

bool ts::DuckChannels::XmlToDVBS(TunerParametersPtr& params, const xml::Element* elem)
{
    TunerParametersDVBS* p = new TunerParametersDVBS;
    params = p;
    return p != nullptr && elem != nullptr &&
        elem->getIntAttribute<size_t>(p->satellite_number, u"satellite", false, 0, 0, 3) &&
        elem->getIntAttribute<uint64_t>(p->frequency, u"frequency", true) &&
        elem->getIntAttribute<uint32_t>(p->symbol_rate, u"symbolrate", false, 27500000) &&
        elem->getIntEnumAttribute(p->modulation, ModulationEnum, u"modulation", false, QPSK) &&
        elem->getIntEnumAttribute(p->delivery_system, DeliverySystemEnum, u"system", false, DS_DVB_S) &&
        elem->getIntEnumAttribute(p->inner_fec, InnerFECEnum, u"FEC", false, FEC_AUTO) &&
        elem->getIntEnumAttribute(p->inversion, SpectralInversionEnum, u"inversion", false, SPINV_AUTO) &&
        elem->getIntEnumAttribute(p->polarity, PolarizationEnum, u"polarity", false, POL_AUTO) &&
        (p->delivery_system == DS_DVB_S || elem->getIntEnumAttribute(p->pilots, PilotEnum, u"pilots", false, PILOT_AUTO)) &&
        (p->delivery_system == DS_DVB_S || elem->getIntEnumAttribute(p->roll_off, RollOffEnum, u"rolloff", false, ROLLOFF_AUTO));
}

bool ts::DuckChannels::XmlToDVBT(TunerParametersPtr& params, const xml::Element* elem)
{
    TunerParametersDVBT* p = new TunerParametersDVBT;
    params = p;
    return p != nullptr && elem != nullptr &&
        elem->getIntAttribute<uint64_t>(p->frequency, u"frequency", true) &&
        elem->getIntEnumAttribute(p->modulation, ModulationEnum, u"modulation", false, QAM_64) &&
        elem->getIntEnumAttribute(p->bandwidth, BandWidthEnum, u"bandwidth", false, BW_AUTO) &&
        elem->getIntEnumAttribute(p->transmission_mode, TransmissionModeEnum, u"transmission", false, TM_AUTO) &&
        elem->getIntEnumAttribute(p->guard_interval, GuardIntervalEnum, u"guard", false, GUARD_AUTO) &&
        elem->getIntEnumAttribute(p->fec_hp, InnerFECEnum, u"HPFEC", false, FEC_AUTO) &&
        elem->getIntEnumAttribute(p->fec_lp, InnerFECEnum, u"LPFEC", false, FEC_AUTO) &&
        elem->getIntEnumAttribute(p->inversion, SpectralInversionEnum, u"inversion", false, SPINV_AUTO) &&
        elem->getIntEnumAttribute(p->hierarchy, HierarchyEnum, u"hierarchy", false, HIERARCHY_NONE) &&
        elem->getIntAttribute<PLP>(p->plp, u"PLP", false, PLP_DISABLE, 0, 255);
}
