//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2024, Thierry Lelegard, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsURILinkageDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"URI_linkage_descriptor"
#define MY_CLASS ts::URILinkageDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_URI_LINKAGE
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::URILinkageDescriptor::URILinkageDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}


void ts::URILinkageDescriptor::DVB_I_Info::clearContent()
{
    end_point_type = 0;
    service_list_name.reset();
    service_list_provider_name.reset();
    private_data.clear();
}

void ts::URILinkageDescriptor::clearContent()
{
    uri_linkage_type = 0;
    uri.clear();
    min_polling_interval = 0;
    private_data.clear();
}

ts::URILinkageDescriptor::URILinkageDescriptor(DuckContext& duck, const Descriptor& desc) :
    URILinkageDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::URILinkageDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::DVB_I_Info::serialize(PSIBuffer& buf) const
{
    buf.putUInt8(end_point_type);
    buf.putStringWithByteLength(service_list_name.value_or(u""));
    buf.putStringWithByteLength(service_list_provider_name.value_or(u""));
    buf.putBytes(private_data);
}

void ts::URILinkageDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putUInt8(uri_linkage_type);
    buf.putStringWithByteLength(uri);
    if (uri_linkage_type == URI_LINKAGE_ONLINE_SDT || uri_linkage_type == URI_LINKAGE_IPTV_SDnS) {
        buf.putUInt16(min_polling_interval);
    }
    else if ((uri_linkage_type == URI_LINKAGE_DVB_I) && dvb_i_private_data.has_value()) {
        dvb_i_private_data.value().serialize(buf);
    }
    if (uri_linkage_type != URI_LINKAGE_DVB_I) {
        buf.putBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::DVB_I_Info::deserialize(PSIBuffer& buf)
{
    end_point_type = buf.getUInt8();
    UString t;
    buf.getStringWithByteLength(t);
    if (!t.empty()) {
        service_list_name = t;
    }
    buf.getStringWithByteLength(t);
    if (!t.empty()) {
        service_list_provider_name = t;
    }
    buf.getBytes(private_data);
}

void ts::URILinkageDescriptor::deserializePayload(PSIBuffer& buf)
{
    uri_linkage_type = buf.getUInt8();
    buf.getStringWithByteLength(uri);
    if (uri_linkage_type == URI_LINKAGE_ONLINE_SDT || uri_linkage_type == URI_LINKAGE_IPTV_SDnS) {
        min_polling_interval = buf.getUInt16();
    }
    else if (uri_linkage_type == URI_LINKAGE_DVB_I) {
        DVB_I_Info inf(buf);
        dvb_i_private_data = inf;
    }
    if (uri_linkage_type != URI_LINKAGE_DVB_I) {
        buf.getBytes(private_data);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------


void ts::URILinkageDescriptor::DVB_I_Info::display(TablesDisplay& disp, PSIBuffer& buf, const UString& margin)
{
    const uint8_t ep_type = buf.getUInt8();
    disp << margin << "End point type: " << DataName(MY_XML_NAME, u"DVB_I_Endpoint_type", ep_type, NamesFlags::HEXA_FIRST) << std::endl;
    UString sl_name = buf.getStringWithByteLength();
    if (!sl_name.empty()) {
        disp << margin << "Service list name: " << sl_name << std::endl;
    }
    UString provider_name = buf.getStringWithByteLength();
    if (!provider_name.empty()) {
        disp << margin << "Service list provider name: " << provider_name << std::endl;
    }
    disp.displayPrivateData(u"Private data", buf, NPOS, margin);
}

void ts::URILinkageDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(2)) {
        const uint8_t type = buf.getUInt8();
        disp << margin << "URI linkage type: " << DataName(MY_XML_NAME, u"LinkageType", type, NamesFlags::HEXA_FIRST) << std::endl;
        disp << margin << "URI: " << buf.getStringWithByteLength() << std::endl;
        if ((type == URI_LINKAGE_ONLINE_SDT || type == URI_LINKAGE_IPTV_SDnS) && buf.canReadBytes(2)) {
            const int interval = buf.getUInt16();
            disp << margin << UString::Format(u"Min polling interval: %d (%d seconds)", {interval, 2 * interval}) << std::endl;
        }
        else if ((type == URI_LINKAGE_DVB_I) && buf.canReadBytes(1)) {
            DVB_I_Info    tmp;
            tmp.display(disp, buf, margin);
        }
        if (type != URI_LINKAGE_DVB_I) {
            disp.displayPrivateData(u"Private data", buf, NPOS, margin);
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::URILinkageDescriptor::DVB_I_Info::toXML(xml::Element* root) const
{
    root->setIntAttribute(u"end_point_type", end_point_type, true);
    if (service_list_name.has_value()) {
        root->setAttribute(u"service_list_name", service_list_name.value());
    }
    if (service_list_provider_name.has_value()) {
        root->setAttribute(u"service_list_provider_name", service_list_provider_name.value());
    }
    if (!private_data.empty()) {
        root->addHexaTextChild(u"private_data", private_data);
    }
}

void ts::URILinkageDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setIntAttribute(u"uri_linkage_type", uri_linkage_type, true);
    root->setAttribute(u"uri", uri);
    if (uri_linkage_type == URI_LINKAGE_ONLINE_SDT || uri_linkage_type == URI_LINKAGE_IPTV_SDnS) {
        root->setIntAttribute(u"min_polling_interval", min_polling_interval);
    }
    else if (uri_linkage_type == URI_LINKAGE_DVB_I) {
        if (dvb_i_private_data.has_value())
            dvb_i_private_data.value().toXML(root->addElement(u"DVB_I_linkage"));
    }
    if ((uri_linkage_type != URI_LINKAGE_DVB_I)  && !private_data.empty()) {
        root->addHexaTextChild(u"private_data", private_data);
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::URILinkageDescriptor::DVB_I_Info::fromXML(const xml::Element* element)
{
    bool ok = element->getIntAttribute(end_point_type, u"end_point_type", true, END_POINT_SERVICE_LIST, END_POINT_MIN, END_POINT_MAX) &&
              element->getHexaTextChild(private_data, u"private_data", false);
    if (ok && (end_point_type == END_POINT_SERVICE_LIST_EXTENDED)) {
        UString slName;
        ok = element->getAttribute(slName, u"service_list_name", true) &&
            element->getOptionalAttribute(service_list_provider_name, u"service_list_provider_name", false);
        service_list_name = slName;
    }
    if (ok) {
        if ((end_point_type != END_POINT_SERVICE_LIST_EXTENDED) && (service_list_name.has_value() || service_list_provider_name.has_value())) {
            element->report().error(u"service_list_name and service_list_provider_name only permitted when end_point_type=0x%X in <%s>, line %d", {END_POINT_SERVICE_LIST_EXTENDED, element->name(), element->lineNumber()});
            ok = false;
        }
    }
    return ok;
}

bool ts::URILinkageDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    bool ok = element->getIntAttribute(uri_linkage_type, u"uri_linkage_type", true) &&
              element->getAttribute(uri, u"uri", true) &&
              element->getIntAttribute(min_polling_interval, u"min_polling_interval", (uri_linkage_type == URI_LINKAGE_ONLINE_SDT || uri_linkage_type == URI_LINKAGE_IPTV_SDnS));
    bool p_ok = true;
    if (uri_linkage_type == URI_LINKAGE_DVB_I) {
        ts::xml::ElementVector el;
        element->getChildren(el, u"private_data");
        if (!el.empty()) {
            element->report().error(u"private_data not permitted when uri_linkage_type=0x%X  in <%s>, line %d", {URI_LINKAGE_DVB_I, element->name(), element->lineNumber()});
            p_ok = false;
        }
    }
    if (ok && uri_linkage_type != URI_LINKAGE_DVB_I) {
        ok = element->getHexaTextChild(private_data, u"private_data", false);
    }
    if (ok && uri_linkage_type == URI_LINKAGE_DVB_I) {
        ts::xml::ElementVector dvb_i_linkage_info;
        DVB_I_Info             dvb_i_url;
        ok = element->getChildren(dvb_i_linkage_info, u"DVB_I_linkage", 1, 1) &&
             dvb_i_url.fromXML(dvb_i_linkage_info[0]);
        if (ok) {
            dvb_i_private_data = dvb_i_url;
        }
    }
    return p_ok && ok;
}
