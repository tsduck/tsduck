//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsRNT.h"
#include "tsBinaryTable.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"RNT"
#define MY_CLASS ts::RNT
#define MY_TID ts::TID_RNT
#define MY_PID ts::PID_RNT
#define MY_STD ts::Standards::DVB

TS_REGISTER_TABLE(MY_CLASS, {MY_TID}, MY_STD, MY_XML_NAME, MY_CLASS::DisplaySection, nullptr, {MY_PID});


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::RNT::RNT(uint8_t version_, bool is_current_) :
    AbstractLongTable(MY_TID, MY_XML_NAME, MY_STD, version_, is_current_),
    descs(this),
    providers(this)
{
}

ts::RNT::RNT(DuckContext& duck, const BinaryTable& table) :
    RNT()
{
    deserialize(duck, table);
}

ts::RNT::ResolutionProvider::ResolutionProvider(const AbstractTable* table) :
    EntryWithDescriptors(table),
    CRID_authorities(table)
{
}

ts::RNT::ResolutionProvider::ResolutionProvider(const AbstractTable* table, const ResolutionProvider& other) :
    EntryWithDescriptors(table, other),
    name(other.name),
    CRID_authorities(table, other.CRID_authorities)
{
}

ts::RNT::CRIDAuthority::CRIDAuthority(const AbstractTable* table) :
    EntryWithDescriptors(table)
{
}

ts::RNT::CRIDAuthority::CRIDAuthority(const AbstractTable* table, const CRIDAuthority& other) :
    EntryWithDescriptors(table, other),
    name(other.name),
    policy(other.policy)
{
}


//----------------------------------------------------------------------------
// Get the table id extension.
//----------------------------------------------------------------------------

uint16_t ts::RNT::tableIdExtension() const
{
    return context_id;
}


//----------------------------------------------------------------------------
// Clear the content of the table.
//----------------------------------------------------------------------------

void ts::RNT::clearContent()
{
    context_id = 0;
    context_id_type = 0;
    descs.clear();
    providers.clear();
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::RNT::deserializePayload(PSIBuffer& buf, const Section& section)
{
    // Get common properties (should be identical in all sections)
    context_id = section.tableIdExtension();
    context_id_type = buf.getUInt8();

    // Accumulate top-level descriptors.
    buf.getDescriptorListWithLength(descs);

    // Loop on resolution providers.
    while (buf.canRead()) {
        ResolutionProvider& rprov(providers.newEntry());

        // Open the resolution provider sequence with length field.
        buf.skipBits(4);
        buf.pushReadSizeFromLength(12);

        // Resolution provider name and descriptors.
        buf.getStringWithByteLength(rprov.name);
        buf.getDescriptorListWithLength(rprov.descs);

        // Loop on CRID authorities.
        while (buf.canRead()) {
            CRIDAuthority& auth(rprov.CRID_authorities.newEntry());
            buf.getStringWithByteLength(auth.name);
            buf.skipBits(2);
            buf.getBits(auth.policy, 2);
            buf.getDescriptorListWithLength(auth.descs);
        }

        // Close the resolution provider sequence with length field.
        buf.popState();
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::RNT::serializePayload(BinaryTable& table, PSIBuffer& buf) const
{
    // Fixed part, to be repeated on all sections.
    buf.putUInt8(context_id_type);
    buf.pushState();

    // Add top-level descriptor list.
    // If the descriptor list is too long to fit into one section, create new sections when necessary.
    for (size_t start = 0;;) {
        start = buf.putPartialDescriptorListWithLength(descs, start);
        if (buf.error() || start >= descs.size()) {
            // Top-level descriptor list completed.
            break;
        }
        else {
            // There are remaining top-level descriptors, flush current section.
            addOneSection(table, buf);
        }
    }

    // Loop on all resolution providers.
    // We make sure that one resolution provider fits inside one section.
    bool retry = false;
    for (auto it_provider = providers.begin(); !buf.error() && it_provider != providers.end();) {
        const ResolutionProvider& prov(it_provider->second);

        // Try to serialize the current provider in the current section.
        // Keep current position in case we cannot completely serialize it.
        buf.pushState();

        // Serialize the resolution provider. Open a sequence with a length field.
        buf.putBits(0xFF, 4);
        buf.pushWriteSequenceWithLeadingLength(12);

        buf.putStringWithByteLength(prov.name);
        buf.putDescriptorListWithLength(prov.descs);
        for (auto it_auth = prov.CRID_authorities.begin(); !buf.error() && it_auth != prov.CRID_authorities.end(); ++it_auth) {
            const CRIDAuthority& auth(it_auth->second);
            buf.putStringWithByteLength(auth.name);
            buf.putBits(0xFF, 2);
            buf.putBits(auth.policy, 2);
            buf.putDescriptorListWithLength(auth.descs);
        }

        // Handle end of serialization for current resolution provider.
        if (!buf.error()) {
            // Provider was successfully serialized.
            retry = false;
            buf.popState();  // close sequence with length field
            buf.dropState(); // drop initially saved position for retry
            ++it_provider;   // move to next provider
        }
        else if (retry) {
            // This is already a retry on an empty section. Definitely too large, invalid table.
            return;
        }
        else {
            // Could not serialize in this section, try with an empty one.
            retry = true;
            buf.dropState();    // drop sequence with length field
            buf.popState();     // return to previous state before current provider
            buf.clearError();   // pretend there was no error when back to retry position
            addOneSection(table, buf);
            buf.putUInt16(0xF000); // empty top-level descriptor list
        }
    }
}


//----------------------------------------------------------------------------
// A static method to display a RNT section.
//----------------------------------------------------------------------------

void ts::RNT::DisplaySection(TablesDisplay& disp, const ts::Section& section, PSIBuffer& buf, const UString& margin)
{
    disp << margin << UString::Format(u"Context id: 0x%X (%<d)", {section.tableIdExtension()}) << std::endl;

    if (buf.canReadBytes(3)) {
        disp << margin << "Context id type: " << DataName(MY_XML_NAME, u"ContextIdType", buf.getUInt8(), NamesFlags::HEXA_FIRST) << std::endl;
        disp.displayDescriptorListWithLength(section, buf, margin, u"RNT top-level descriptors:", u"None");

        // Loop on resolution providers.
        while (buf.canReadBytes(3)) {
            // Open the resolution provider sequence with length field.
            buf.skipBits(4);
            buf.pushReadSizeFromLength(12);

            disp << margin << "- Resolution provider name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
            disp.displayDescriptorListWithLength(section, buf, margin + u"  ", u"Provider-level descriptors:", u"None");

            // Loop on CRID authorities.
            while (buf.canReadBytes(1)) {
                disp << margin << "  - CRID authority name: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
                if (buf.canReadBytes(1)) {
                    buf.skipBits(2);
                    disp << margin << "    CRID authority policy: " << DataName(MY_XML_NAME, u"AuthorityPolicy", buf.getBits<uint8_t>(2), NamesFlags::DECIMAL_FIRST) << std::endl;
                    disp.displayDescriptorListWithLength(section, buf, margin + u"    ", u"CRID authority-level descriptors:", u"None");
                }
            }

            // Close the resolution provider sequence with length field.
            disp.displayPrivateData(u"Extraneous data", buf, NPOS, margin);
            buf.popState();
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::RNT::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"version", version);
    root->setBoolAttribute(u"current", is_current);
    root->setIntAttribute(u"context_id", context_id, true);
    root->setIntAttribute(u"context_id_type", context_id_type, true);
    descs.toXML(duck, root);
    for (const auto& it1 : providers) {
        xml::Element* e1 = root->addElement(u"resolution_provider");
        e1->setAttribute(u"name", it1.second.name);
        it1.second.descs.toXML(duck, e1);
        for (const auto& it2 : it1.second.CRID_authorities) {
            xml::Element* e2 = e1->addElement(u"CRID_authority");
            e2->setAttribute(u"name", it2.second.name);
            e2->setIntAttribute(u"policy", it2.second.policy);
            it2.second.descs.toXML(duck, e2);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::RNT::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xprov;
    bool ok =
        element->getIntAttribute(version, u"version", false, 0, 0, 31) &&
        element->getBoolAttribute(is_current, u"current", false, true) &&
        element->getIntAttribute(context_id, u"context_id", true) &&
        element->getIntAttribute(context_id_type, u"context_id_type", true) &&
        descs.fromXML(duck, xprov, element, u"resolution_provider");

    for (auto it1 = xprov.begin(); ok && it1 != xprov.end(); ++it1) {
        ResolutionProvider& rprov(providers.newEntry());
        xml::ElementVector xauth;
        ok = (*it1)->getAttribute(rprov.name, u"name", true, UString(), 0, 255) &&
             rprov.descs.fromXML(duck, xauth, *it1, u"CRID_authority");
        for (auto it2 = xauth.begin(); ok && it2 != xauth.end(); ++it2) {
            CRIDAuthority& auth(rprov.CRID_authorities.newEntry());
            ok = (*it2)->getAttribute(auth.name, u"name", true, UString(), 0, 255) &&
                 (*it2)->getIntAttribute(auth.policy, u"policy", true, 0, 0, 3) &&
                 auth.descs.fromXML(duck, *it2);
        }
    }
    return ok;
}
