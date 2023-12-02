//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2023, Paul Higgs
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
#define MY_CLASS ts::DefaultAuthorityDescriptor
#define MY_DID ts::DID_DEFAULT_AUTHORITY
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::DefaultAuthorityDescriptor::DefaultAuthorityDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
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

static ts::ByteBlock fromString(std::string s)
{
    ts::ByteBlock b {};
    for (std::string::iterator it = s.begin(); it != s.end(); ++it)
        b.push_back(*it);
    return b;
}

static std::string fromByteBlock(ts::ByteBlock b)
{
    std::string s {};
    for (auto ch : b) {
        s.push_back(ch);
    }
    return s;
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

void ts::DefaultAuthorityDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    ByteBlock da;
    buf.getBytes(da);
    disp.displayVector(u"Default authority: ", da, margin, true, 16);
    std::string s = fromByteBlock(da);
    disp << margin << "  fqdn: \"" << s << "\"" <<  std::endl;
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
