//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAbstractDownloadContentDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::AbstractDownloadContentDescriptor::~AbstractDownloadContentDescriptor()
{
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AbstractDownloadContentDescriptor::Module::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt16(module_id);
    buf.putUInt32(module_size);
    buf.putUInt8(uint8_t(module_info.size()));
    buf.putBytes(module_info);
}

void ts::AbstractDownloadContentDescriptor::TextInfo::serializePayload(PSIBuffer& buf) const
{
    buf.putLanguageCode(ISO_639_language_code);
    buf.putStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AbstractDownloadContentDescriptor::Module::deserializePayload(PSIBuffer& buf)
{
    module_id = buf.getUInt16();
    module_size = buf.getUInt32();
    buf.getBytes(module_info, buf.getUInt8());
}

void ts::AbstractDownloadContentDescriptor::TextInfo::deserializePayload(PSIBuffer& buf)
{
    buf.getLanguageCode(ISO_639_language_code);
    buf.getStringWithByteLength(text);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

bool ts::AbstractDownloadContentDescriptor::Module::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(7);
    if (ok) {
        disp << margin << UString::Format(u"Module id: %n", buf.getUInt16());
        disp << UString::Format(u", size: %'d bytes", buf.getUInt32()) << std::endl;
        const size_t count = buf.getUInt8();
        ok = buf.canReadBytes(count);
        disp.displayPrivateData(u"Module info", buf, count, margin);
    }
    return ok;
}

bool ts::AbstractDownloadContentDescriptor::TextInfo::Display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    bool ok = buf.canReadBytes(4);
    if (ok) {
        disp << margin << "Language: " << buf.getLanguageCode() << std::endl;
        disp << margin << "Text: \"" << buf.getStringWithByteLength() << "\"" << std::endl;
    }
    return ok;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AbstractDownloadContentDescriptor::Module::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"module");
    e->setIntAttribute(u"module_id", module_id, true);
    e->setIntAttribute(u"module_size", module_size);
    e->addHexaTextChild(u"module_info", module_info, true);
}

void ts::AbstractDownloadContentDescriptor::TextInfo::buildXML(DuckContext& duck, xml::Element* root) const
{
    xml::Element* e = root->addElement(u"text_info");
    e->setAttribute(u"ISO_639_language_code", ISO_639_language_code);
    e->setAttribute(u"text", text);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AbstractDownloadContentDescriptor::Module::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(module_id, u"module_id", true) &&
           element->getIntAttribute(module_size, u"module_size", true) &&
           element->getHexaTextChild(module_info, u"module_info");
}

bool ts::AbstractDownloadContentDescriptor::TextInfo::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getAttribute(ISO_639_language_code, u"ISO_639_language_code", true, u"", 3, 3) &&
           element->getAttribute(text, u"text", true);
}
