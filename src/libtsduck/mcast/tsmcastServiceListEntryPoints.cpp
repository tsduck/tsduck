//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsmcastServiceListEntryPoints.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::ServiceListEntryPoints(Report& report, const FluteFile& file) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"ServiceListEntryPoints", true)) {
        const xml::Element* root = doc.rootElement();

        // Decode root attributes.
        _valid = root->getIntAttribute(version, u"version", false) && root->getAttribute(lang, u"lang", true);

        // Decode all ServiceListRegistryEntity elements.
        for (const xml::Element* e = root->findFirstChild(u"ServiceListRegistryEntity", false); _valid && e != nullptr; e = e->findNextSibling(true)) {
            entities.emplace_back(e);
            _valid = entities.back().valid;
        }

        // Decode all ProviderOffering elements.
        for (const xml::Element* e = root->findFirstChild(u"ProviderOffering", false); _valid && e != nullptr; e = e->findNextSibling(true)) {
            providers.emplace_back(e);
            _valid = providers.back().valid;
        }
    }
}

ts::mcast::ServiceListEntryPoints::~ServiceListEntryPoints()
{
}


//----------------------------------------------------------------------------
// Definition of a <ProviderOffering> element.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::ProviderOffering::ProviderOffering(const xml::Element* element) :
    provider(element, u"Provider")
{
    if (element != nullptr) {
        valid = provider.valid;
        for (const xml::Element* e = element->findFirstChild(u"ServiceListOffering", false); valid && e != nullptr; e = e->findNextSibling(true)) {
            lists.emplace_back(e);
            valid = lists.back().valid;
        }
    }
}


//----------------------------------------------------------------------------
// Definition of a <ServiceListOffering> element in a <ProviderOffering>.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::ServiceListOffering::ServiceListOffering(const xml::Element* element)
{
    if (element != nullptr) {
        valid = element->getBoolAttribute(regulator, u"regulatorListFlag") &&
                element->getAttribute(lang, u"lang") &&
                element->getTextChild(name, u"ServiceListName", true, true) &&
                element->getTextChild(list_id, u"ServiceListId", true, true);
        for (const xml::Element* e = element->findFirstChild(u"ServiceListURI", false); valid && e != nullptr; e = e->findNextSibling(true)) {
            lists.emplace_back(e);
            valid = lists.back().valid;
        }
    }
}


//----------------------------------------------------------------------------
// Constructor of an OrganizationType element.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::Organization::Organization(const xml::Element* parent, const UString& element) :
    Organization(parent == nullptr ? nullptr : parent->findFirstChild(element))
{
}

ts::mcast::ServiceListEntryPoints::Organization::Organization(const xml::Element* element)
{
    if (element != nullptr) {
        valid = element->getBoolAttribute(regulator, u"regulatorFlag");

        // Get all <Name> until we find one with type "main".
        UString type;
        for (const xml::Element* e = element->findFirstChild(u"Name", false); valid && e != nullptr && !type.similar(u"main"); e = e->findNextSibling(true)) {
            valid = e->getText(name, true) && e->getAttribute(type, u"type");
        }

        // Ignore all other elements for now...
    }
}


//----------------------------------------------------------------------------
// Definition of an ExtendedURIType or ExtendedURIPathType element.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::ExtendedURI::ExtendedURI(const xml::Element* parent, const UString& element) :
    ExtendedURI(parent == nullptr ? nullptr : parent->findFirstChild(element))
{
}

ts::mcast::ServiceListEntryPoints::ExtendedURI::ExtendedURI(const xml::Element* element)
{
    if (element != nullptr) {
        valid = element->getTextChild(uri, u"URI", true, true) && element->getAttribute(type, u"contentType", true);
    }
}
