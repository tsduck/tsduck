//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2025, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUWAVideoStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"CUVV_video_stream_descriptor"
#define MY_CLASS    ts::UWAVideoStreamDescriptor
#define MY_EDID     ts::EDID::PrivateMPEG(ts::DID_CUVV_HDR, ts::REGID_CUVV)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::UWAVideoStreamDescriptor::UWAVideoStreamDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

void ts::UWAVideoStreamDescriptor::clearContent()
{
    cuvv_tag = 0;
    cuva_version_map = 0;
    terminal_provide_code = 0;
    terminal_provide_oriented_code = 0;
}

ts::UWAVideoStreamDescriptor::UWAVideoStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    UWAVideoStreamDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::UWAVideoStreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt32(cuvv_tag);
    buf.putUInt16(cuva_version_map);
    buf.putUInt16(terminal_provide_code);
    buf.putInt16(int16_t(terminal_provide_oriented_code));
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::UWAVideoStreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    cuvv_tag = buf.getUInt32();
    cuva_version_map = buf.getUInt16();
    terminal_provide_code = buf.getUInt16();
    terminal_provide_oriented_code = buf.getInt16();
}


//----------------------------------------------------------------------------
// Thread-safe init-safe static data patterns.
//----------------------------------------------------------------------------

const ts::Names& ts::UWAVideoStreamDescriptor::VersionNumbers()
{
    static const Names data({
        {u"1.0", 0x0005},
        {u"2.0", 0x0006},
        {u"3.0", 0x0007},
        {u"4.0", 0x0008},
    });
    return data;
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::UWAVideoStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    if (buf.canReadBytes(10)) {
        disp << margin << "CUVV Tag: " << DataName(MY_XML_NAME, u"CUVV_tag", buf.getUInt32(), NamesFlags::NAME_VALUE | NamesFlags::HEXA);
        uint16_t _version_map = buf.getUInt16();
        disp << ", provider code: " << UString::Format(u"0x%x", buf.getUInt16());
        disp << ", provider oriented code: " << DataName(MY_XML_NAME, u"terminal_provide_oriented_code", buf.getInt16(), NamesFlags::NAME_VALUE | NamesFlags::HEXA) << std::endl;

        std::vector<int8_t> versions;
        for (uint8_t i = 0; i < 16; i++) {
            if (_version_map & (1 << i)) {
                versions.push_back(i + 1);
            }
        }
        if (versions.empty()) {
            disp << margin << "No versions specified" << std::endl;
        }
        else {
            disp.displayVector(u"Version Map:", versions, margin, true, 8);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::UWAVideoStreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"cuvv_tag", cuvv_tag, true);
    root->setIntAttribute(u"cuva_version_map", cuva_version_map, true);
    root->setIntAttribute(u"terminal_provide_code", terminal_provide_code, true);
    root->setEnumAttribute(VersionNumbers(), u"terminal_provide_oriented_code", terminal_provide_oriented_code);
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::UWAVideoStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    return element->getIntAttribute(cuvv_tag, u"cuvv_tag", true, ts::REGID_CUVV, ts::REGID_CUVV, ts::REGID_CUVV) &&
           element->getIntAttribute(cuva_version_map, u"cuva_version_map", true) &&
           element->getIntAttribute(terminal_provide_code, u"terminal_provide_code", true, 0x0004, 0x0004, 0x0004) &&
           element->getEnumAttribute(terminal_provide_oriented_code, VersionNumbers(), u"terminal_provide_oriented_code", true, 0x0005);
}
