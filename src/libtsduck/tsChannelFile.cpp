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

#include "tsChannelFile.h"
#include "tsModulation.h"
#include "tsxmlElement.h"
#include "tsSysUtils.h"
TSDUCK_SOURCE;


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::ChannelFile::ChannelFile() :
    _networks(),
    _xmlTweaks(),
    _fileName()
{
}

ts::ChannelFile::Service::Service(uint16_t id_) :
    id(id_),
    name(),
    provider(),
    lcn(),
    pmtPID(),
    type(),
    cas()
{
}

ts::ChannelFile::TransportStream::TransportStream(uint16_t id_, uint16_t onid_, const TunerParametersPtr& tune_) :
    id(id_),
    onid(onid_),
    tune(tune_),
    _services()
{
}

ts::ChannelFile::Network::Network(uint16_t id_, TunerType type_) :
    id(id_),
    type(type_),
    _ts()
{
}


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
    for (size_t i = 0; i < _services.size(); ++i) {
        const ServicePtr& srv(_services[i]);
        assert(!srv.isNull());
        if ((strict && srv->name == name) || (!strict && name.similar(srv->name))) {
            return srv;
        }
    }
    return ServicePtr(); // not found, null pointer.
}


//----------------------------------------------------------------------------
// Add a service in a transport stream.
//----------------------------------------------------------------------------

bool ts::ChannelFile::TransportStream::addService(const ServicePtr& srv, CopyShare copy, bool replace)
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
                _services[i] = copy == SHARE ? srv : new Service(*srv);
                CheckNonNull(_services[i].pointer());
                return true;
            }
            else {
                return false;
            }
        }
    }

    // Add new service.
    _services.push_back(copy == SHARE ? srv : new Service(*srv));
    CheckNonNull(_services.back().pointer());
    return true;
}


//----------------------------------------------------------------------------
// Add a list of services in the transport stream.
//----------------------------------------------------------------------------

void ts::ChannelFile::TransportStream::addServices(const ServiceList& list)
{
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it->hasId()) {
            ServicePtr srv(serviceGetOrCreate(it->getId()));
            if (it->hasName()) {
                srv->name = it->getName();
            }
            if (it->hasProvider()) {
                srv->provider = it->getProvider();
            }
            if (it->hasLCN()) {
                srv->lcn = it->getLCN();
            }
            if (it->hasPMTPID()) {
                srv->pmtPID = it->getPMTPID();
            }
            if (it->hasType()) {
                srv->type = it->getType();
            }
            if (it->hasCAControlled()) {
                srv->cas = it->getCAControlled();
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
        if (net->id == id && net->type == type) {
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

ts::TunerParametersPtr ts::ChannelFile::serviceToTuningInternal(TunerType type, const UString& name, bool strict, bool useTunerType, Report& report) const
{
    NetworkPtr net;
    TransportStreamPtr ts;
    ServicePtr srv;
    return searchServiceInternal(net, ts, srv, type, name, strict, useTunerType, report) ? ts->tune : TunerParametersPtr();
}

bool ts::ChannelFile::searchServiceInternal(NetworkPtr& net,
                                            TransportStreamPtr& ts,
                                            ServicePtr& srv,
                                            TunerType type,
                                            const UString& name,
                                            bool strict,
                                            bool useTunerType,
                                            Report& report) const
{
    // Clear output parameters.
    net.clear();
    ts.clear();
    srv.clear();

    // Loop through all networks.
    for (size_t inet = 0; inet < _networks.size(); ++inet) {
        const NetworkPtr& pnet(_networks[inet]);
        assert(!pnet.isNull());
        if (!useTunerType || pnet->type == type) {
            // Inspect this network, loop through all transport stream.
            for (size_t its = 0; its < pnet->tsCount(); ++its) {
                const TransportStreamPtr& pts(pnet->tsByIndex(its));
                assert(!pts.isNull());
                srv = pts->serviceByName(name, strict);
                if (!srv.isNull()) {
                    net = pnet;
                    ts = pts;
                    return true;
                }
            }
        }
    }

    // Channel not found.
    if (_fileName.empty()) {
        report.error(u"channel \"%s\" not found in channel database", {name});
    }
    else {
        report.error(u"channel \"%s\" not found in file %s", {name, _fileName});
    }
    return false;
}


//----------------------------------------------------------------------------
// Default XML channel file name.
//----------------------------------------------------------------------------

ts::UString ts::ChannelFile::DefaultFileName()
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

bool ts::ChannelFile::load(const UString& fileName, Report& report)
{
    clear();
    _fileName = fileName;
    xml::Document doc(report);
    doc.setTweaks(_xmlTweaks);
    return doc.load(fileName, false) && parseDocument(doc);
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
    xml::Document model(doc.report());
    if (!model.load(u"tsduck.channels.model.xml", true)) {
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
        _networks.push_back(net);

        // Get network properties.
        xml::ElementVector xts;
        success =
            (*itnet)->getIntAttribute<uint16_t>(net->id, u"id", true) &&
            (*itnet)->getIntEnumAttribute(net->type, TunerTypeEnum, u"type", true) &&
            (*itnet)->getChildren(xts, u"ts") &&
            success;

        // Get all TS in the network.
        for (auto itts = xts.begin(); itts != xts.end(); ++itts) {

            // Get transport stream properties.
            uint16_t tsid = 0;
            uint16_t onid = 0;
            bool tsOk =
                (*itts)->getIntAttribute<uint16_t>(tsid, u"id", true) &&
                (*itts)->getIntAttribute<uint16_t>(onid, u"onid", true);
            success = tsOk && success;

            if (tsOk) {
                // Build a new TransportStream object.
                const TransportStreamPtr ts(net->tsGetOrCreate(tsid));
                assert(!ts.isNull());
                ts->onid = onid;

                // Loop on all children elements. Exactly one should be tuner parameters, others must be <service>.
                for (const xml::Element* e = (*itts)->firstChildElement(); e != nullptr; e = e->nextSiblingElement()) {
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
                            success;

                        // Add the service in the transport stream.
                        ts->addService(srv, SHARE, true);
                    }
                    else if (!ts->tune.isNull()) {
                        // Tuner parameters already set.
                        doc.report().error(u"Invalid <%s> at line %d, at most one set of tuner parameters is allowed in <ts>", {e->name(), e->lineNumber()});
                        success = false;
                    }
                    else {
                        // Try to set tuning parameters from this element.
                        ts->tune = TunerParameters::FromXML(e);
                        if (ts->tune.isNull()) {
                            doc.report().error(u"Invalid <%s> at line %d", {e->name(), e->lineNumber()});
                            success = false;
                        }
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
        if (!IsDirectory(dir)) {
            const ErrorCode err = CreateDirectory(dir, true);
            if (err != SYS_SUCCESS) {
                report.error(u"error creating directory %s: %s", {dir, ErrorCodeMessage(err)});
            }
        }
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
    for (auto itnet = _networks.begin(); itnet != _networks.end(); ++itnet) {
        const NetworkPtr& net(*itnet);
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
            xts->setIntAttribute(u"onid", ts->onid, true);

            // Set tuner parameters. No error if null (this is just an incomplete description).
            if (!ts->tune.isNull()) {
                ts->tune->toXML(xts);
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
            }
        }
    }
    return true;
}
