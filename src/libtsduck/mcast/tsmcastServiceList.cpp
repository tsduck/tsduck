//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastServiceList.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::mcast::ServiceList::ServiceList(Report& report, const FluteFile& file) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"ServiceList", true)) {
        const xml::Element* root = doc.rootElement();

        _valid = root->getIntAttribute(version, u"version", true) &&
                 root->getAttribute(list_id, u"id", true) &&
                 root->getAttribute(lang, u"lang", true) &&
                 root->getTextChild(list_name, u"Name", true, true) &&
                 root->getTextChild(provider_name, u"ProviderName", true, true);

        for (const xml::Element* e = root->findFirstChild(u"Service", true); _valid && e != nullptr; e = e->findNextSibling(true)) {
            services.emplace_back(e, false);
            _valid = services.back().valid;
        }

        for (const xml::Element* e = root->findFirstChild(u"TestService", true); _valid && e != nullptr; e = e->findNextSibling(true)) {
            services.emplace_back(e, true);
            _valid = services.back().valid;
        }

        for (const xml::Element* e1 = root->findFirstChild(u"LCNTableList", true); _valid && e1 != nullptr; e1 = e1->findNextSibling(true)) {
            for (const xml::Element* e2 = e1->findFirstChild(u"LCNTable"); _valid && e2 != nullptr; e2 = e2->findNextSibling(true)) {
                lcn_tables.emplace_back(e2);
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

ts::mcast::ServiceList::LCNTable::LCNTable(const xml::Element* element)
{
    if (element != nullptr) {
        valid = element->getBoolAttribute(preserve_broadcast_lcn, u"preserveBroadcastLCN");
        for (const xml::Element* e = element->findFirstChild(u"LCN", true); valid && e != nullptr; e = e->findNextSibling(true)) {
            lcns.emplace_back();
            auto& lcn(lcns.back());
            valid = e->getBoolAttribute(lcn.visible, u"visible", false, true) &&
                    e->getBoolAttribute(lcn.selectable, u"selectable", false, true) &&
                    e->getIntAttribute(lcn.channel_number, u"channelNumber", true) &&
                    e->getAttribute(lcn.service_ref, u"serviceRef", true);
        }
    }
}


//----------------------------------------------------------------------------
// Definition of a <Service> or <TestService>.
//----------------------------------------------------------------------------

ts::mcast::ServiceList::ServiceType::ServiceType(const xml::Element* element, bool test) :
    test_service(test)
{
    if (element != nullptr) {
        valid = element->getIntAttribute(version, u"version", true) &&
                element->getAttribute(lang, u"lang") &&
                element->getBoolAttribute(dynamic, u"dynamic") &&
                element->getBoolAttribute(replay_available, u"replayAvailable") &&
                element->getTextChild(unique_id, u"UniqueIdentifier", true, true) &&
                element->getTextChild(service_name, u"ServiceName", true, true) &&
                element->getTextChild(provider_name, u"ProviderName", true, true);

        for (const xml::Element* e = element->findFirstChild(u"ServiceInstance", true); valid && e != nullptr; e = e->findNextSibling(true)) {
            instances.emplace_back();
            auto& inst(instances.back());
            valid = e->getIntAttribute(inst.priority, u"priority") &&
                    e->getAttribute(inst.id, u"id") &&
                    e->getAttribute(inst.lang, u"lang");

            const xml::Element* e1 = e->findFirstChild(u"IdentifierBasedDeliveryParameters", true);
            if (valid && e1 != nullptr) {
                valid = e1->getText(inst.id_based_params, true) &&
                        e1->getAttribute(inst.id_based_params_type, u"contentType");
            }

            if (valid &&
                (e1 = e->findFirstChild(u"OtherDeliveryParameters", true)) != nullptr &&
                e1->getAttribute(inst.other_params_ext_name, u"extensionName") &&
                inst.other_params_ext_name == u"vnd.apple.mpegurl" &&
                (e1 = e1->findFirstChild(u"UriBasedLocation", true)) != nullptr)
            {
                e1->getTextChild(inst.other_params_uri, u"URI", true);
                e1->getAttribute(inst.other_params_uri_type, u"contentType");
            }
        }
    }
}
