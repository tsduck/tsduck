//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDSMCCCompatibilityDescriptor.h"
#include "tsNames.h"
#include "tsxmlElement.h"
#include "tsOUI.h"


//----------------------------------------------------------------------------
// Total number of bytes to serialize the compatibilityDescriptor().
//----------------------------------------------------------------------------

size_t ts::DSMCCCompatibilityDescriptor::binarySize() const
{
    size_t size = 4; // header
    for (const auto& desc : descs) {
        size += 11; // fixed part
        for (const auto& subdesc : desc.subdescs) {
            size += 2 + subdesc.additionalInformation.size();
        }
    }
    return size;
}


//----------------------------------------------------------------------------
// Serialize the compatibilityDescriptor().
//----------------------------------------------------------------------------

void ts::DSMCCCompatibilityDescriptor::serialize(PSIBuffer& buf, bool zero_size_if_empty) const
{
    if (zero_size_if_empty && descs.empty()) {
        // Generate a zero-size structure.
        buf.putUInt16(0);
    }
    else {
        buf.pushWriteSequenceWithLeadingLength(16); // reserve compatibilityDescriptorLength
        buf.putUInt16(uint16_t(descs.size()));
        for (const auto& desc : descs) {
            buf.putUInt8(desc.descriptorType);
            buf.pushWriteSequenceWithLeadingLength(8); // reserve descriptorLength
            buf.putUInt8(desc.specifierType);
            buf.putUInt24(desc.specifierData);
            buf.putUInt16(desc.model);
            buf.putUInt16(desc.version);
            buf.putUInt8(uint8_t(desc.subdescs.size()));
            for (const auto& subdesc : desc.subdescs) {
                buf.putUInt8(subdesc.subDescriptorType);
                buf.putUInt8(uint8_t(subdesc.additionalInformation.size()));
                buf.putBytes(subdesc.additionalInformation);
            }
            buf.popState(); // update descriptorLength
        }
        buf.popState(); // update compatibilityDescriptorLength
    }
}


//----------------------------------------------------------------------------
// Deserialize the compatibilityDescriptor().
//----------------------------------------------------------------------------

void ts::DSMCCCompatibilityDescriptor::deserialize(PSIBuffer& buf)
{
    descs.clear();
    buf.pushReadSizeFromLength(16);
    // Read a 16-bit number of descriptors.
    // Accept that the data length is zero, meaning no descriptor, not even a number of descriptors.
    if (buf.canRead()) {
        size_t descriptorCount = buf.getUInt16();
        while (buf.canRead() && descriptorCount-- > 0) {
            Descriptor& desc(descs.emplace_back());
            desc.descriptorType = buf.getUInt8();
            buf.pushReadSizeFromLength(8);
            desc.specifierType = buf.getUInt8();
            desc.specifierData = buf.getUInt24();
            desc.model = buf.getUInt16();
            desc.version = buf.getUInt16();
            size_t subDescriptorCount = buf.getUInt8();
            while (buf.canRead() && subDescriptorCount-- > 0) {
                SubDescriptor& subdesc(desc.subdescs.emplace_back());
                subdesc.subDescriptorType = buf.getUInt8();
                buf.getBytes(subdesc.additionalInformation, buf.getUInt8());
            }
            buf.popState();
        }
    }
    buf.popState();
}


//----------------------------------------------------------------------------
// A static method to display a compatibilityDescriptor().
//----------------------------------------------------------------------------

bool ts::DSMCCCompatibilityDescriptor::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    if (!buf.canReadBytes(2)) {
        return false;
    }
    buf.pushReadSizeFromLength(16);
    if (buf.canReadBytes(2)) {
        const size_t descriptorCount = buf.getUInt16();
        disp << margin << "DSM-CC compatibility descriptor: " << descriptorCount << " descriptors" << std::endl;
        for (size_t i = 0; i < descriptorCount && buf.canReadBytes(11); ++i) {
            disp << margin << "- Descriptor #" << i << ", type: " << NameFromSection(u"dtv", u"DSMCC.descriptorType", buf.getUInt8(), NamesFlags::HEX_VALUE_NAME) << std::endl;
            buf.pushReadSizeFromLength(8);
            const uint8_t specifierType = buf.getUInt8();
            disp << margin
                 << "  Specifier type: " << NameFromSection(u"dtv", u"DSMCC.specifierType", specifierType, NamesFlags::HEX_VALUE_NAME)
                 << ", specifier data: ";
            if (specifierType == DSMCC_SPTYPE_OUI) {
                disp << OUIName(buf.getUInt24(), NamesFlags::HEX_VALUE_NAME);
            }
            else {
                disp << UString::Format(u"%n", buf.getUInt24());
            }
            disp << std::endl << margin << UString::Format(u"  Model: %n", buf.getUInt16());
            disp << UString::Format(u", version: %n", buf.getUInt16()) << std::endl;
            const size_t subDescriptorCount = buf.getUInt8();
            disp << margin << "  Number of subdescriptors: " << subDescriptorCount << std::endl;
            for (size_t subi = 0; subi < subDescriptorCount && buf.canReadBytes(2); ++subi) {
                disp << margin << UString::Format(u"  - Subdescriptor #%d, type: %n", subi, buf.getUInt8()) << std::endl;
                disp.displayPrivateData(u"Additional information", buf, buf.getUInt8(), margin + u"    ");
            }
            buf.popState();
        }
    }
    disp.displayPrivateData(u"Extraneous data in compatibility descriptor", buf, NPOS, margin);
    buf.popState();
    return !buf.error();
}


//----------------------------------------------------------------------------
// This method converts a compatibilityDescriptor() to XML.
//----------------------------------------------------------------------------

ts::xml::Element* ts::DSMCCCompatibilityDescriptor::toXML(DuckContext& duck, xml::Element* parent, bool only_not_empty, const UChar* xml_name) const
{
    if (only_not_empty && descs.empty()) {
        return nullptr;
    }
    xml::Element* element = parent->addElement(xml_name);
    for (const auto& desc : descs) {
        xml::Element* xdesc = element->addElement(u"descriptor");
        xdesc->setIntAttribute(u"descriptorType", desc.descriptorType, true);
        xdesc->setIntAttribute(u"specifierType", desc.specifierType, true);
        xdesc->setIntAttribute(u"specifierData", desc.specifierData, true);
        xdesc->setIntAttribute(u"model", desc.model, true);
        xdesc->setIntAttribute(u"version", desc.version, true);
        for (const auto& subdesc : desc.subdescs) {
            xml::Element* xsubdesc = xdesc->addElement(u"subDescriptor");
            xsubdesc->setIntAttribute(u"subDescriptorType", subdesc.subDescriptorType, true);
            xsubdesc->addHexaText(subdesc.additionalInformation, true);
        }
    }
    return element;
}


//----------------------------------------------------------------------------
// This method decodes an XML compatibilityDescriptor().
//----------------------------------------------------------------------------

bool ts::DSMCCCompatibilityDescriptor::fromXML(DuckContext& duck, const xml::Element* parent, bool required, const UChar* xml_name)
{
    descs.clear();

    // Get the compatibilityDescriptor() element.
    const xml::Element* e = parent;
    if (xml_name != nullptr) {
        xml::ElementVector children;
        if (!parent->getChildren(children, xml_name, required ? 1 : 0, 1)) {
            return false;
        }
        if (children.empty()) { // implies required == false
            return true;
        }
        e = children[0];
    }

    // Analyze the compatibilityDescriptor() element.
    xml::ElementVector xdescs;
    bool ok = e->getChildren(xdescs, u"descriptor");
    for (size_t i1 = 0; ok && i1 < xdescs.size(); ++i1) {
        Descriptor& desc(descs.emplace_back());
        xml::ElementVector xsubdescs;
        ok = xdescs[i1]->getIntAttribute(desc.descriptorType, u"descriptorType", true) &&
             xdescs[i1]->getIntAttribute(desc.specifierType, u"specifierType", false, 1) &&
             xdescs[i1]->getIntAttribute(desc.specifierData, u"specifierData", true, 0, 0, 0x00FFFFFF) &&
             xdescs[i1]->getIntAttribute(desc.model, u"model", false, 0) &&
             xdescs[i1]->getIntAttribute(desc.version, u"version", false, 0) &&
             xdescs[i1]->getChildren(xsubdescs, u"subDescriptor");
        for (size_t i2 = 0; ok && i2 < xsubdescs.size(); ++i2) {
            SubDescriptor& subdesc(desc.subdescs.emplace_back());
            ok = xsubdescs[i2]->getIntAttribute(subdesc.subDescriptorType, u"subDescriptorType", true) &&
                 xsubdescs[i2]->getHexaText(subdesc.additionalInformation);
        }
    }
    return ok;
}
