//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastServiceList.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::mcast::ServiceList::ServiceList(Report& report, const FluteFile& file, bool strict) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"ServiceList", true)) {
        const xml::Element* root = doc.rootElement();

        // Display error but do not fail if missing (some bogus streams don't have them).
        root->getIntAttribute(version, u"version", strict);
        root->getAttribute(list_id, u"id", strict);
        root->getAttribute(lang, u"lang", strict);

        // The others must really be there.
        _valid = root->getTextChild(list_name, u"Name", true, strict) &&
                 root->getTextChild(provider_name, u"ProviderName", true, strict);

        for (auto& e : root->children(u"Service", &_valid)) {
            services.emplace_back(&e, false, strict);
            _valid = services.back().valid;
        }

        for (auto& e : root->children(u"TestService", &_valid)) {
            services.emplace_back(&e, true, strict);
            _valid = services.back().valid;
        }

        for (auto& e1 : root->children(u"LCNTableList", &_valid)) {
            for (auto& e2 : e1.children(u"LCNTable", &_valid)) {
                lcn_tables.emplace_back(&e2, strict);
                _valid = lcn_tables.back().valid;
            }
        }
    }
}

ts::mcast::ServiceList::~ServiceList()
{
}


//----------------------------------------------------------------------------
// Definition of a <LCNTableList>.
//----------------------------------------------------------------------------

ts::mcast::ServiceList::LCNTable::LCNTable(const xml::Element* element, bool strict)
{
    if (element != nullptr) {
        valid = element->getBoolAttribute(preserve_broadcast_lcn, u"preserveBroadcastLCN");
        for (auto& e : element->children(u"LCN", &valid)) {
            lcns.emplace_back();
            auto& lcn(lcns.back());
            valid = e.getBoolAttribute(lcn.visible, u"visible", false, true) &&
                    e.getBoolAttribute(lcn.selectable, u"selectable", false, true) &&
                    e.getIntAttribute(lcn.channel_number, u"channelNumber", strict) &&
                    e.getAttribute(lcn.service_ref, u"serviceRef", strict);
        }
    }
}


//----------------------------------------------------------------------------
// Definition of a <Service> or <TestService>.
//----------------------------------------------------------------------------

ts::mcast::ServiceList::ServiceType::ServiceType(const xml::Element* element, bool test, bool strict) :
    test_service(test)
{
    const xml::Element* e1 = nullptr;
    UString ext_name;

    if (element != nullptr) {
        // Display error but do not fail if missing (some bogus streams don't have them).
        element->getIntAttribute(version, u"version", strict);
        element->getAttribute(lang, u"lang");
        element->getBoolAttribute(dynamic, u"dynamic");
        element->getBoolAttribute(replay_available, u"replayAvailable");

        // The others must really be there.
        valid = element->getTextChild(unique_id, u"UniqueIdentifier", true, strict) &&
                element->getTextChild(service_name, u"ServiceName", true, strict) &&
                element->getTextChild(provider_name, u"ProviderName", true, strict);

        // Service type is optional.
        if ((e1 = element->findFirstChild(u"ServiceType")) != nullptr) {
            valid = e1->getAttribute(service_type, u"href");
        }

        // Loop on all "service instances" (various places where the same service is available).
        for (auto& e : element->children(u"ServiceInstance", &valid)) {
            instances.emplace_back();
            auto& inst(instances.back());
            valid = e.getIntAttribute(inst.priority, u"priority") &&
                    e.getAttribute(inst.id, u"id") &&
                    e.getAttribute(inst.lang, u"lang");

            // Try to find a playlist in <IdentifierBasedDeliveryParameters>.
            if (valid &&
                (e1 = e.findFirstChild(u"IdentifierBasedDeliveryParameters")) != nullptr)
            {
                valid = e1->getText(inst.media_params, true) &&
                        e1->getAttribute(inst.media_params_type, u"contentType");
            }

            // Otherwise, try to find a manifest in <DASHDeliveryParameters>.
            if (valid &&
                inst.media_params.empty() &&
                (e1 = e.findFirstChild(u"DASHDeliveryParameters")) != nullptr &&
                (e1 = e1->findFirstChild(u"UriBasedLocation")) != nullptr)
            {
                e1->getTextChild(inst.media_params, u"URI", strict);
                e1->getAttribute(inst.media_params_type, u"contentType");
            }

            // Otherwise, try to find a playlist in <OtherDeliveryParameters>.
            // Not sure it is valid but it appeared in at least one example.
            if (valid &&
                inst.media_params.empty() &&
                (e1 = e.findFirstChild(u"OtherDeliveryParameters")) != nullptr &&
                e1->getAttribute(ext_name, u"extensionName") &&
                ext_name == u"vnd.apple.mpegurl" &&
                (e1 = e1->findFirstChild(u"UriBasedLocation")) != nullptr)
            {
                e1->getTextChild(inst.media_params, u"URI", strict);
                e1->getAttribute(inst.media_params_type, u"contentType");
            }
        }
    }
}
