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

ts::mcast::ServiceListEntryPoints::ServiceListEntryPoints(Report& report, const FluteFile& file, bool strict) :
    FluteFile(file)
{
    // Parse the XML document.
    xml::Document doc(report);
    if (parseXML(doc, u"ServiceListEntryPoints", true)) {
        const xml::Element* root = doc.rootElement();

        // Decode root attributes.
        _valid = root->getIntAttribute(version, u"version", false) && root->getAttribute(lang, u"lang", strict);

        // Decode all ServiceListRegistryEntity elements.
        for (const xml::Element* e = root->findFirstChild(u"ServiceListRegistryEntity", !strict); _valid && e != nullptr; e = e->findNextSibling(true)) {
            entities.emplace_back(e, strict);
            _valid = entities.back().valid;
        }

        // Decode all ProviderOffering elements.
        for (const xml::Element* e = root->findFirstChild(u"ProviderOffering", !strict); _valid && e != nullptr; e = e->findNextSibling(true)) {
            providers.emplace_back(e, strict);
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

ts::mcast::ServiceListEntryPoints::ProviderOffering::ProviderOffering(const xml::Element* element, bool strict) :
    provider(element, u"Provider", strict)
{
    if (element != nullptr) {
        valid = provider.valid;
        for (const xml::Element* e = element->findFirstChild(u"ServiceListOffering", !strict); valid && e != nullptr; e = e->findNextSibling(true)) {
            lists.emplace_back(e, strict);
            valid = lists.back().valid;
        }
    }
}


//----------------------------------------------------------------------------
// Definition of a <ServiceListOffering> element in a <ProviderOffering>.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::ServiceListOffering::ServiceListOffering(const xml::Element* element, bool strict)
{
    if (element != nullptr) {
        valid = element->getBoolAttribute(regulator, u"regulatorListFlag") &&
                element->getAttribute(lang, u"lang") &&
                element->getTextChild(name, u"ServiceListName", true, strict) &&
                element->getTextChild(list_id, u"ServiceListId", true, strict);
        for (const xml::Element* e = element->findFirstChild(u"ServiceListURI", !strict); valid && e != nullptr; e = e->findNextSibling(true)) {
            lists.emplace_back(e, strict);
            valid = lists.back().valid;
        }
    }
}


//----------------------------------------------------------------------------
// Constructor of an OrganizationType element.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::Organization::Organization(const xml::Element* parent, const UString& element, bool strict) :
    Organization(parent == nullptr ? nullptr : parent->findFirstChild(element, !strict))
{
}

ts::mcast::ServiceListEntryPoints::Organization::Organization(const xml::Element* element, bool strict)
{
    if (element != nullptr) {
        valid = element->getBoolAttribute(regulator, u"regulatorFlag");

        // Get all <Name> until we find one with type "main".
        UString type;
        for (const xml::Element* e = element->findFirstChild(u"Name", !strict); valid && e != nullptr && !type.similar(u"main"); e = e->findNextSibling(true)) {
            valid = e->getText(name, true) && e->getAttribute(type, u"type");
        }

        // Ignore all other elements for now...
    }
}


//----------------------------------------------------------------------------
// Definition of an ExtendedURIType or ExtendedURIPathType element.
//----------------------------------------------------------------------------

ts::mcast::ServiceListEntryPoints::ExtendedURI::ExtendedURI(const xml::Element* parent, const UString& element, bool strict) :
    ExtendedURI(parent == nullptr ? nullptr : parent->findFirstChild(element, !strict))
{
}

ts::mcast::ServiceListEntryPoints::ExtendedURI::ExtendedURI(const xml::Element* element, bool strict)
{
    if (element != nullptr) {
        valid = element->getTextChild(uri, u"URI", true, strict) && element->getAttribute(type, u"contentType", strict);
    }
}
