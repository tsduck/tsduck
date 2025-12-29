//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023-2026, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsDefaultAuthorityDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIBuffer.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"default_authority_descriptor"
#define MY_CLASS    ts::DefaultAuthorityDescriptor
#define MY_EDID     ts::EDID::Regular(ts::DID_DVB_DEFAULT_AUTHORITY, ts::Standards::DVB)

TS_REGISTER_DESCRIPTOR(MY_CLASS, MY_EDID, MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DefaultAuthorityDescriptor::DefaultAuthorityDescriptor() :
    AbstractDescriptor(MY_EDID, MY_XML_NAME)
{
}

ts::DefaultAuthorityDescriptor::DefaultAuthorityDescriptor(DuckContext& duck, const Descriptor& desc) :
    DefaultAuthorityDescriptor()
{
    deserialize(duck, desc);
}

void ts::DefaultAuthorityDescriptor::clearContent()
{
    default_authority.clear();
}


//----------------------------------------------------------------------------
// Helpers
//----------------------------------------------------------------------------

namespace {
    ts::ByteBlock fromString(const std::string& s)
    {
        ts::ByteBlock b;
        for (auto it = s.begin(); it != s.end(); ++it) {
            b.push_back(*it);
        }
        return b;
    }

    std::string fromByteBlock(const ts::ByteBlock& b)
    {
        std::string s;
        for (auto ch : b) {
            s.push_back(ch);
        }
        return s;
    }
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::DefaultAuthorityDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBytes(fromString(default_authority));
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::DefaultAuthorityDescriptor::deserializePayload(PSIBuffer& buf)
{
    ByteBlock da;
    buf.getBytes(da);
    default_authority = fromByteBlock(da);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::DefaultAuthorityDescriptor::DisplayDescriptor(TablesDisplay& disp, const ts::Descriptor& desc, PSIBuffer& buf, const UString& margin, const ts::DescriptorContext& context)
{
    ByteBlock da;
    buf.getBytes(da);
    disp.displayVector(u"Default authority: ", da, margin, true, 16);
    disp << margin << "  fqdn: \"" << fromByteBlock(da) << "\"" << std::endl;
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::DefaultAuthorityDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setAttribute(u"fqdn", UString::FromUTF8(default_authority));
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::DefaultAuthorityDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    UString da;
    bool ok = element->getAttribute(da, u"fqdn", true);
    if (ok) {
        default_authority = da.toUTF8();
    }
    return ok;
}
