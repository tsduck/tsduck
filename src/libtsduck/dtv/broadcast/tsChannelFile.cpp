//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsChannelFile.h"
#include "tsModulation.h"
#include "tsLegacyBandWidth.h"
#include "tsxmlModelDocument.h"
#include "tsxmlElement.h"
#include "tsFileUtils.h"
#include "tsErrCodeReport.h"
#include "tsFatal.h"


//----------------------------------------------------------------------------
// Transport stream accessors.
//----------------------------------------------------------------------------

ts::ChannelFile::ServicePtr ts::ChannelFile::TransportStream::serviceByIndex(size_t index) const
{
    return index < _services.size() ? _services[index] : ServicePtr();
}

ts::ChannelFile::ServicePtr ts::ChannelFile::TransportStream::serviceById(uint16_t id_) const
{
    for (size_t i = 0; i < _services.size(); ++i) {
        const ServicePtr& srv(_services[i]);
        assert(!srv.isNull());
        if (srv->id == id_) {
            return srv;
        }
    }
    return ServicePtr(); // not found, null pointer.
}

ts::ChannelFile::ServicePtr ts::ChannelFile::TransportStream::serviceGetOrCreate(uint16_t id_)
{
    // Try to get an existing service.
    ServicePtr srv(serviceById(id_));
    if (srv.isNull()) {
        // Not found, create a new service.
        srv = new Service(id_);
        CheckNonNull(srv.pointer());
        _services.push_back(srv);
    }
    return srv;
}

ts::ChannelFile::ServicePtr ts::ChannelFile::TransportStream::serviceByName(const UString& name, bool strict) const
{
    // Check if the name has "major.minor" syntax.
    uint16_t majorId = 0;
    uint16_t minorId = 0;
    const bool atscId = !strict && name.scan(u"%d.%d", {&majorId, &minorId});

    // Now lookup all services in transport.
    for (size_t i = 0; i < _services.size(); ++i) {
        const ServicePtr& srv(_services[i]);
        assert(!srv.isNull());
        if ((strict && srv->name == name) ||
            (!strict && name.similar(srv->name)) ||
            (atscId && srv->atscMajorId == majorId && srv->atscMinorId == minorId))
        {
            return srv;
        }
    }
    return ServicePtr(); // not found, null pointer.
}


//----------------------------------------------------------------------------
// Add a service in a transport stream.
//----------------------------------------------------------------------------

bool ts::ChannelFile::TransportStream::addService(const ServicePtr& srv, ShareMode copy, bool replace)
{
    // Filter out null pointer.
    if (srv.isNull()) {
        return false;
    }

    // Look for a service with same id.
    for (size_t i = 0; i < _services.size(); ++i) {
        assert(!_services[i].isNull());
        if (_services[i]->id == srv->id) {
            if (replace) {
                _services[i] = copy == ShareMode::SHARE ? srv : new Service(*srv);
                CheckNonNull(_services[i].pointer());
                return true;
            }
            else {
                return false;
            }
        }
    }

    // Add new service.
    _services.push_back(copy == ShareMode::SHARE ? srv : new Service(*srv));
    CheckNonNull(_services.back().pointer());
    return true;
}


//----------------------------------------------------------------------------
// Add a list of services in the transport stream.
//----------------------------------------------------------------------------

void ts::ChannelFile::TransportStream::addServices(const ServiceList& list)
{
    for (const auto& it : list) {
        if (it.hasId()) {
            ServicePtr srv(serviceGetOrCreate(it.getId()));
            if (it.hasName()) {
                srv->name = it.getName();
            }
            if (it.hasProvider()) {
                srv->provider = it.getProvider();
            }
            if (it.hasLCN()) {
                srv->lcn = it.getLCN();
            }
            if (it.hasPMTPID()) {
                srv->pmtPID = it.getPMTPID();
            }
            if (it.hasTypeDVB()) {
                srv->type = it.getTypeDVB();
            }
            if (it.hasCAControlled()) {
                srv->cas = it.getCAControlled();
            }
            if (it.hasTypeATSC()) {
                srv->atscType = it.getTypeATSC();
            }
            if (it.hasMajorIdATSC()) {
                srv->atscMajorId = it.getMajorIdATSC();
            }
            if (it.hasMinorIdATSC()) {
                srv->atscMinorId = it.getMinorIdATSC();
            }
        }
    }
}


//----------------------------------------------------------------------------
// Network accessors.
//----------------------------------------------------------------------------

ts::ChannelFile::TransportStreamPtr ts::ChannelFile::Network::tsByIndex(size_t index) const
{
    return index < _ts.size() ? _ts[index] : TransportStreamPtr();
}

ts::ChannelFile::TransportStreamPtr ts::ChannelFile::Network::tsById(uint16_t id_) const
{
    for (size_t i = 0; i < _ts.size(); ++i) {
        const TransportStreamPtr& ts(_ts[i]);
        assert(!ts.isNull());
        if (ts->id == id_) {
            return ts;
        }
    }
    return TransportStreamPtr(); // not found, null pointer.
}

ts::ChannelFile::TransportStreamPtr ts::ChannelFile::Network::tsGetOrCreate(uint16_t id_)
{
    // Try to get an existing transport stream.
    TransportStreamPtr ts(tsById(id_));
    if (ts.isNull()) {
        // Not found, create a new TS.
        ts = new TransportStream(id_);
        CheckNonNull(ts.pointer());
        _ts.push_back(ts);
    }
    return ts;
}


//----------------------------------------------------------------------------
// Network lookup.
//----------------------------------------------------------------------------

ts::ChannelFile::NetworkPtr ts::ChannelFile::networkByIndex(size_t index) const
{
    return index < _networks.size() ? _networks[index] : NetworkPtr();
}

ts::ChannelFile::NetworkPtr ts::ChannelFile::networkById(uint16_t id, TunerType type) const
{
    for (size_t i = 0; i < _networks.size(); ++i) {
        const NetworkPtr& net(_networks[i]);
        assert(!net.isNull());
        if (net->id == id && (type == TT_UNDEFINED || net->type == type)) {
            return net;
        }
    }
    return NetworkPtr(); // not found, null pointer.
}

ts::ChannelFile::NetworkPtr ts::ChannelFile::networkGetOrCreate(uint16_t id, TunerType type)
{
    // Try to get an existing transport stream.
    NetworkPtr net(networkById(id, type));
    if (net.isNull()) {
        // Not found, create a new network.
        net = new Network(id, type);
        CheckNonNull(net.pointer());
        _networks.push_back(net);
    }
    return net;
}


//----------------------------------------------------------------------------
// Search a service by name in any network of a given type of the file.
//----------------------------------------------------------------------------

bool ts::ChannelFile::serviceToTuning(ModulationArgs& tune, const DeliverySystemSet& delsys, const UString& name, bool strict, Report& report) const
{
    NetworkPtr net;
    TransportStreamPtr ts;
    ServicePtr srv;
    if (searchService(net, ts, srv, delsys, name, strict, report)) {
        tune = ts->tune;
        return true;
    }
    else {
        return false;
    }
}

bool ts::ChannelFile::searchService(NetworkPtr& net,
                                    TransportStreamPtr& ts,
                                    ServicePtr& srv,
                                    const DeliverySystemSet& delsys,
                                    const UString& name,
                                    bool strict,
                                    Report& report) const
{
    report.debug(u"searching channel \"%s\" for delivery systems %s in %s", {name, delsys, fileDescription()});

    // Clear output parameters.
    net.clear();
    ts.clear();
    srv.clear();

    // Loop through all networks.
    for (size_t inet = 0; inet < _networks.size(); ++inet) {

        const NetworkPtr& pnet(_networks[inet]);
        assert(!pnet.isNull());

        // Inspect this network, loop through all transport stream.
        for (size_t its = 0; its < pnet->tsCount(); ++its) {
            const TransportStreamPtr& pts(pnet->tsByIndex(its));
            assert(!pts.isNull());
            // Check if this TS has an acceptable delivery system.
            // If the input delsys is empty, accept any delivery system.
            if (delsys.empty() || (pts->tune.delivery_system.has_value() && delsys.contains(pts->tune.delivery_system.value()))) {
                report.debug(u"searching channel \"%s\" in TS id 0x%X, delivery system %s", {name, pts->id, DeliverySystemEnum.name(pts->tune.delivery_system.value_or(DS_UNDEFINED))});
                srv = pts->serviceByName(name, strict);
                if (!srv.isNull()) {
                    report.debug(u"found channel \"%s\" in TS id 0x%X", {name, pts->id});
                    net = pnet;
                    ts = pts;
                    return true;
                }
            }
        }
    }

    // Channel not found.
    report.error(u"channel \"%s\" not found in %s", {name, fileDescription()});
    return false;
}


//----------------------------------------------------------------------------
// Get a description of the file from which the channel database was loaded.
//----------------------------------------------------------------------------

ts::UString ts::ChannelFile::fileDescription() const
{
    return _fileName.empty() ? u"channel database" : _fileName;
}


//----------------------------------------------------------------------------
// Default XML channel file name.
//----------------------------------------------------------------------------

ts::UString ts::ChannelFile::DefaultFileName()
{
    return UserConfigurationFileName(u".tsduck.channels.xml", u"channels.xml");
}


//----------------------------------------------------------------------------
// Load an XML file or text.
//----------------------------------------------------------------------------

bool ts::ChannelFile::load(const UString& fileName, Report& report)
{
    clear();
    _fileName = fileName.empty() ? DefaultFileName() : fileName;
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(_fileName, false) && parseDocument(doc);
}

bool ts::ChannelFile::load(std::istream& strm, Report& report)
{
    clear();
    _fileName.clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(strm) && parseDocument(doc);
}

bool ts::ChannelFile::parse(const UString& text, Report& report)
{
    clear();
    _fileName.clear();
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.parse(text) && parseDocument(doc);
}


//----------------------------------------------------------------------------
// Parse an XML document.
//----------------------------------------------------------------------------

bool ts::ChannelFile::parseDocument(const xml::Document& doc)
{
    // Load the XML model for TSDuck files. Search it in TSDuck directory.
    xml::ModelDocument model(doc.report());
    if (!model.load(u"tsduck.channels.model.xml", true)) {
        doc.report().error(u"Model for TSDuck channels XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!model.validate(doc)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();
    bool success = true;

    // Analyze all networks in the document.
    assert(root != nullptr);
    xml::ElementVector xnets;
    root->getChildren(xnets, u"network");
    for (auto itnet : xnets) {

        // Build a new Network object at end of our list of networks.
        const NetworkPtr net(new Network);
        CheckNonNull(net.pointer());
        _networks.push_back(net);

        // Get network properties.
        xml::ElementVector xts;
        success =
            itnet->getIntAttribute<uint16_t>(net->id, u"id", true) &&
            itnet->getIntEnumAttribute(net->type, TunerTypeEnum, u"type", true) &&
            itnet->getChildren(xts, u"ts") &&
            success;

        // Get all TS in the network.
        for (auto itts : xts) {

            // Get transport stream properties.
            uint16_t tsid = 0;
            uint16_t onid = 0;
            bool tsOk =
                itts->getIntAttribute<uint16_t>(tsid, u"id", true) &&
                itts->getIntAttribute<uint16_t>(onid, u"onid", false, 0xFFFF);
            success = tsOk && success;

            if (tsOk) {
                // Build a new TransportStream object.
                const TransportStreamPtr ts(net->tsGetOrCreate(tsid));
                assert(!ts.isNull());
                ts->onid = onid;

                // Loop on all children elements. Exactly one should be tuner parameters, others must be <service>.
                for (const xml::Element* e = itts->firstChildElement(); e != nullptr; e = e->nextSiblingElement()) {
                    if (e->name().similar(u"service")) {
                        // Get a service description.
                        const ServicePtr srv(new Service);
                        CheckNonNull(srv.pointer());

                        // Get service properties.
                        success =
                            e->getIntAttribute<uint16_t>(srv->id, u"id", true) &&
                            e->getAttribute(srv->name, u"name", false) &&
                            e->getAttribute(srv->provider, u"provider", false) &&
                            e->getOptionalIntAttribute(srv->lcn, u"LCN") &&
                            e->getOptionalIntAttribute(srv->pmtPID, u"PMTPID", PID(0), PID(PID_NULL)) &&
                            e->getOptionalIntAttribute(srv->type, u"type") &&
                            e->getOptionalBoolAttribute(srv->cas, u"cas") &&
                            e->getOptionalIntAttribute<uint8_t>(srv->atscType, u"atsc_type", 0, 0x3F) &&
                            e->getOptionalIntAttribute<uint16_t>(srv->atscMajorId, u"atsc_major_id", 0, 0x03FF) &&
                            e->getOptionalIntAttribute<uint16_t>(srv->atscMinorId, u"atsc_minor_id", 0, 0x03FF) &&
                            success;

                        // Add the service in the transport stream.
                        ts->addService(srv, ShareMode::SHARE, true);
                    }
                    else if (ts->tune.hasModulationArgs()) {
                        // Tuner parameters already set.
                        doc.report().error(u"Invalid <%s> at line %d, at most one set of tuner parameters is allowed in <ts>", {e->name(), e->lineNumber()});
                        success = false;
                    }
                    else if (!fromXML(ts->tune, e, net->type, tsid)) {
                        doc.report().error(u"Invalid <%s> at line %d", {e->name(), e->lineNumber()});
                        success = false;
                    }
                }
            }
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Create XML file or text.
//----------------------------------------------------------------------------

bool ts::ChannelFile::save(const UString& fileName, bool createDirectories, Report& report) const
{
    if (createDirectories) {
        const UString dir(DirectoryName(fileName));
        fs::create_directories(dir, &ErrCodeReport(report, u"error creating directory", dir));
    }

    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) && doc.save(fileName);
}

ts::UString ts::ChannelFile::toXML(Report& report) const
{
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return generateDocument(doc) ? doc.toString() : UString();
}


//----------------------------------------------------------------------------
// Generate an XML document.
//----------------------------------------------------------------------------

bool ts::ChannelFile::generateDocument(xml::Document& doc) const
{
    // Initialize the document structure.
    xml::Element* root = doc.initialize(u"tsduck");
    if (root == nullptr) {
        return false;
    }

    // Format all networks.
    for (const auto& itnet : _networks) {
        const NetworkPtr& net(itnet);
        assert(!net.isNull());

        // Create one network element.
        xml::Element* xnet = root->addElement(u"network");
        xnet->setIntAttribute(u"id", net->id, true);
        xnet->setEnumAttribute(TunerTypeEnum, u"type", net->type);

        // Format all transport streams.
        for (size_t its = 0; its < net->tsCount(); ++its) {
            const TransportStreamPtr& ts(net->tsByIndex(its));
            assert(!ts.isNull());

            // Create one transport stream element.
            xml::Element* xts = xnet->addElement(u"ts");
            xts->setIntAttribute(u"id", ts->id, true);
            if (ts->onid != 0xFFFF) {
                xts->setIntAttribute(u"onid", ts->onid, true);
            }

            // Set tuner parameters. No error if unset (this is just an incomplete description).
            if (ts->tune.hasModulationArgs()) {
                toXML(ts->tune, xts);
            }

            // Format all services.
            for (size_t isrv = 0; isrv < ts->serviceCount(); ++isrv) {
                const ServicePtr& srv(ts->serviceByIndex(isrv));
                assert(!srv.isNull());

                // Create one service element.
                xml::Element* xsrv = xts->addElement(u"service");
                xsrv->setIntAttribute(u"id", srv->id, true);
                xsrv->setAttribute(u"name", srv->name, true);
                xsrv->setAttribute(u"provider", srv->provider, true);
                xsrv->setOptionalIntAttribute(u"LCN", srv->lcn, false);
                xsrv->setOptionalIntAttribute(u"PMTPID", srv->pmtPID, true);
                xsrv->setOptionalIntAttribute(u"type", srv->type, true);
                xsrv->setOptionalBoolAttribute(u"cas", srv->cas);
                xsrv->setOptionalIntAttribute(u"atsc_type", srv->atscType, true);
                xsrv->setOptionalIntAttribute(u"atsc_major_id", srv->atscMajorId, false);
                xsrv->setOptionalIntAttribute(u"atsc_minor_id", srv->atscMinorId, false);
            }
        }
    }
    return true;
}


//----------------------------------------------------------------------------
// Convert modulation parameters from XML.
//----------------------------------------------------------------------------

bool ts::ChannelFile::fromXML(ModulationArgs& mod, const xml::Element* elem, TunerType tunerType, uint16_t ts_id)
{
    // Clear parameter area.
    mod.clear();

    // Handle all expected modulation arguments structures.
    if (elem == nullptr) {
        return false;
    }
    else if (elem->name().similar(u"dvbs")) {
        mod.delivery_system = DS_DVB_S;
        return elem->getOptionalIntAttribute(mod.satellite_number, u"satellite", 0, 3) &&
               elem->getVariableIntAttribute(mod.frequency, u"frequency", true) &&
               elem->getVariableIntAttribute(mod.symbol_rate, u"symbolrate", false, 27500000) &&
               elem->getVariableIntEnumAttribute(mod.modulation, ModulationEnum, u"modulation", false, QPSK) &&
               elem->getVariableIntEnumAttribute(mod.delivery_system, DeliverySystemEnum, u"system", false, DS_DVB_S) &&
               elem->getOptionalIntEnumAttribute(mod.inner_fec, InnerFECEnum, u"FEC") &&
               elem->getOptionalIntEnumAttribute(mod.inversion, SpectralInversionEnum, u"inversion") &&
               elem->getOptionalIntEnumAttribute(mod.polarity, PolarizationEnum, u"polarity") &&
               (mod.delivery_system == DS_DVB_S || elem->getOptionalIntEnumAttribute(mod.pilots, PilotEnum, u"pilots")) &&
               (mod.delivery_system == DS_DVB_S || elem->getOptionalIntEnumAttribute(mod.roll_off, RollOffEnum, u"rolloff")) &&
               (mod.delivery_system == DS_DVB_S || elem->getOptionalIntAttribute<uint32_t>(mod.isi, u"isi")) &&
               (mod.delivery_system == DS_DVB_S || elem->getOptionalIntAttribute<uint32_t>(mod.pls_code, u"PLS_code")) &&
               (mod.delivery_system == DS_DVB_S || elem->getOptionalIntEnumAttribute(mod.pls_mode, PLSModeEnum, u"PLS_mode"));
    }
    else if (elem->name().similar(u"dvbt")) {
        mod.delivery_system = DS_DVB_T;
        return elem->getVariableIntAttribute(mod.frequency, u"frequency", true) &&
               elem->getVariableIntEnumAttribute(mod.modulation, ModulationEnum, u"modulation", false, QAM_64) &&
               GetLegacyBandWidth(mod.bandwidth, elem, u"bandwidth") &&
               elem->getOptionalIntEnumAttribute(mod.transmission_mode, TransmissionModeEnum, u"transmission") &&
               elem->getOptionalIntEnumAttribute(mod.guard_interval, GuardIntervalEnum, u"guard") &&
               elem->getOptionalIntEnumAttribute(mod.fec_hp, InnerFECEnum, u"HPFEC") &&
               elem->getOptionalIntEnumAttribute(mod.fec_lp, InnerFECEnum, u"LPFEC") &&
               elem->getOptionalIntEnumAttribute(mod.inversion, SpectralInversionEnum, u"inversion") &&
               elem->getOptionalIntEnumAttribute(mod.hierarchy, HierarchyEnum, u"hierarchy") &&
               elem->getOptionalIntAttribute(mod.plp, u"PLP", 0, 255);
    }
    else if (elem->name().similar(u"dvbc")) {
        mod.delivery_system = DS_DVB_C;
        return elem->getVariableIntAttribute(mod.frequency, u"frequency", true) &&
               elem->getVariableIntAttribute(mod.symbol_rate, u"symbolrate", false, 6900000) &&
               elem->getVariableIntEnumAttribute(mod.modulation, ModulationEnum, u"modulation", false, QAM_64) &&
               elem->getOptionalIntEnumAttribute(mod.inner_fec, InnerFECEnum, u"FEC") &&
               elem->getOptionalIntEnumAttribute(mod.inversion, SpectralInversionEnum, u"inversion");
    }
    else if (elem->name().similar(u"atsc")) {
        mod.delivery_system = DS_ATSC;
        return elem->getVariableIntAttribute(mod.frequency, u"frequency", true) &&
               elem->getVariableIntEnumAttribute(mod.modulation, ModulationEnum, u"modulation", false, VSB_8) &&
               elem->getOptionalIntEnumAttribute(mod.inversion, SpectralInversionEnum, u"inversion");
    }
    else if (elem->name().similar(u"isdbt")) {
        mod.delivery_system = DS_ISDB_T;
        return elem->getVariableIntAttribute(mod.frequency, u"frequency", true) &&
               GetLegacyBandWidth(mod.bandwidth, elem, u"bandwidth") &&
               elem->getOptionalIntEnumAttribute(mod.transmission_mode, TransmissionModeEnum, u"transmission") &&
               elem->getOptionalIntEnumAttribute(mod.guard_interval, GuardIntervalEnum, u"guard") &&
               elem->getOptionalIntEnumAttribute(mod.inversion, SpectralInversionEnum, u"inversion");
    }
    else if (elem->name().similar(u"isdbs")) {
        mod.delivery_system = DS_ISDB_S;
        mod.stream_id = ts_id;
        return elem->getOptionalIntAttribute(mod.satellite_number, u"satellite", 0, 3) &&
               elem->getVariableIntAttribute(mod.frequency, u"frequency", true) &&
               elem->getVariableIntAttribute(mod.symbol_rate, u"symbolrate", false, 27500000) &&
               elem->getOptionalIntEnumAttribute(mod.inner_fec, InnerFECEnum, u"FEC") &&
               elem->getOptionalIntEnumAttribute(mod.inversion, SpectralInversionEnum, u"inversion") &&
               elem->getOptionalIntEnumAttribute(mod.polarity, PolarizationEnum, u"polarity");
    }
    else {
        // Not a valid modulation parameters node.
        return false;
    }
}


//----------------------------------------------------------------------------
// Convert modulation parameters to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::ChannelFile::toXML(const ModulationArgs& mod, xml::Element* parent) const
{
    const DeliverySystem delsys = mod.delivery_system.value_or(DS_UNDEFINED);

    switch (TunerTypeOf(delsys)) {
        case TT_DVB_S: {
            xml::Element* e = parent->addElement(u"dvbs");
            if (mod.satellite_number.has_value() && *mod.satellite_number != 0) {
                e->setOptionalIntAttribute(u"satellite", mod.satellite_number, false);
            }
            e->setOptionalIntAttribute(u"frequency", mod.frequency, false);
            e->setOptionalIntAttribute(u"symbolrate", mod.symbol_rate, false);
            e->setOptionalEnumAttribute(ModulationEnum, u"modulation", mod.modulation);
            if (delsys != DS_DVB_S) {
                e->setOptionalEnumAttribute(DeliverySystemEnum, u"system", mod.delivery_system);
            }
            if (mod.polarity != POL_AUTO) {
                e->setOptionalEnumAttribute(PolarizationEnum, u"polarity", mod.polarity);
            }
            if (mod.inversion != SPINV_AUTO) {
                e->setOptionalEnumAttribute(SpectralInversionEnum, u"inversion", mod.inversion);
            }
            if (mod.inner_fec != FEC_AUTO) {
                e->setOptionalEnumAttribute(InnerFECEnum, u"FEC", mod.inner_fec);
            }
            if (delsys == DS_DVB_S2 && mod.pilots != PILOT_AUTO) {
                e->setOptionalEnumAttribute(PilotEnum, u"pilots", mod.pilots);
            }
            if (delsys == DS_DVB_S2 && mod.roll_off != ROLLOFF_AUTO) {
                e->setOptionalEnumAttribute(RollOffEnum, u"rolloff", mod.roll_off);
            }
            if (delsys == DS_DVB_S2 && mod.isi != ISI_DISABLE) {
                e->setOptionalIntAttribute(u"ISI", mod.isi, false);
                e->setOptionalIntAttribute(u"PLS_code", mod.pls_code, false);
                e->setOptionalEnumAttribute(PLSModeEnum, u"PLS_mode", mod.pls_mode);
            }
            return e;
        }
        case TT_DVB_T: {
            xml::Element* e = parent->addElement(u"dvbt");
            e->setOptionalIntAttribute(u"frequency", mod.frequency, false);
            if (mod.modulation != QAM_AUTO) {
                e->setOptionalEnumAttribute(ModulationEnum, u"modulation", mod.modulation);
            }
            if (mod.fec_hp != FEC_AUTO) {
                e->setOptionalEnumAttribute(InnerFECEnum, u"HPFEC", mod.fec_hp);
            }
            if (mod.fec_lp != FEC_AUTO) {
                e->setOptionalEnumAttribute(InnerFECEnum, u"LPFEC", mod.fec_lp);
            }
            if (mod.bandwidth != BW_AUTO) {
                e->setOptionalIntAttribute(u"bandwidth", mod.bandwidth);
            }
            if (mod.transmission_mode != TM_AUTO) {
                e->setOptionalEnumAttribute(TransmissionModeEnum, u"transmission", mod.transmission_mode);
            }
            if (mod.guard_interval != GUARD_AUTO) {
                e->setOptionalEnumAttribute(GuardIntervalEnum, u"guard", mod.guard_interval);
            }
            if (mod.hierarchy != HIERARCHY_AUTO) {
                e->setOptionalEnumAttribute(HierarchyEnum, u"hierarchy", mod.hierarchy);
            }
            if (mod.plp != PLP_DISABLE) {
                e->setOptionalIntAttribute(u"PLP", mod.plp, false);
            }
            if (mod.inversion != SPINV_AUTO) {
                e->setOptionalEnumAttribute(SpectralInversionEnum, u"inversion", mod.inversion);
            }
            return e;
        }
        case TT_DVB_C: {
            xml::Element* e = parent->addElement(u"dvbc");
            e->setOptionalIntAttribute(u"frequency", mod.frequency, false);
            e->setOptionalIntAttribute(u"symbolrate", mod.symbol_rate, false);
            e->setOptionalEnumAttribute(ModulationEnum, u"modulation", mod.modulation);
            if (mod.inner_fec != FEC_AUTO) {
                e->setOptionalEnumAttribute(InnerFECEnum, u"FEC", mod.inner_fec);
            }
            if (mod.inversion != SPINV_AUTO) {
                e->setOptionalEnumAttribute(SpectralInversionEnum, u"inversion", mod.inversion);
            }
            return e;
        }
        case TT_ATSC: {
            xml::Element* e = parent->addElement(u"atsc");
            e->setOptionalIntAttribute(u"frequency", mod.frequency, false);
            e->setOptionalEnumAttribute(ModulationEnum, u"modulation", mod.modulation);
            if (mod.inversion != SPINV_AUTO) {
                e->setOptionalEnumAttribute(SpectralInversionEnum, u"inversion", mod.inversion);
            }
            return e;
        }
        case TT_ISDB_T: {
            xml::Element* e = parent->addElement(u"isdbt");
            e->setOptionalIntAttribute(u"frequency", mod.frequency, false);
            if (mod.bandwidth != BW_AUTO) {
                e->setOptionalIntAttribute(u"bandwidth", mod.bandwidth);
            }
            if (mod.transmission_mode != TM_AUTO) {
                e->setOptionalEnumAttribute(TransmissionModeEnum, u"transmission", mod.transmission_mode);
            }
            if (mod.guard_interval != GUARD_AUTO) {
                e->setOptionalEnumAttribute(GuardIntervalEnum, u"guard", mod.guard_interval);
            }
            if (mod.inversion != SPINV_AUTO) {
                e->setOptionalEnumAttribute(SpectralInversionEnum, u"inversion", mod.inversion);
            }
            return e;
        }
        case TT_ISDB_S: {
            xml::Element* e = parent->addElement(u"isdbs");
            if (mod.satellite_number.has_value() && *mod.satellite_number != 0) {
                e->setOptionalIntAttribute(u"satellite", mod.satellite_number, false);
            }
            e->setOptionalIntAttribute(u"frequency", mod.frequency, false);
            e->setOptionalIntAttribute(u"symbolrate", mod.symbol_rate, false);
            if (mod.polarity != POL_AUTO) {
                e->setOptionalEnumAttribute(PolarizationEnum, u"polarity", mod.polarity);
            }
            if (mod.inversion != SPINV_AUTO) {
                e->setOptionalEnumAttribute(SpectralInversionEnum, u"inversion", mod.inversion);
            }
            if (mod.inner_fec != FEC_AUTO) {
                e->setOptionalEnumAttribute(InnerFECEnum, u"FEC", mod.inner_fec);
            }
            return e;
        }
        case TT_ISDB_C:
        case TT_UNDEFINED:
        default: {
            return nullptr;
        }
    }
}
