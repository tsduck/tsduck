//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMPEGH3DAudioMultiStreamDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MPEGH_3D_audio_multi_stream_descriptor"
#define MY_CLASS ts::MPEGH3DAudioMultiStreamDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_MPH3D_MULTI
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MPEGH3DAudioMultiStreamDescriptor::MPEGH3DAudioMultiStreamDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MPEGH3DAudioMultiStreamDescriptor::clearContent()
{
    this_is_main_stream = false;
    this_stream_id = 0;
    num_auxiliary_streams = 0;
    mae_groups.clear();
    reserved.clear();
}

ts::MPEGH3DAudioMultiStreamDescriptor::MPEGH3DAudioMultiStreamDescriptor(DuckContext& duck, const Descriptor& desc) :
    MPEGH3DAudioMultiStreamDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MPEGH3DAudioMultiStreamDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioMultiStreamDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBit(this_is_main_stream);
    buf.putBits(this_stream_id, 7);
    if (this_is_main_stream) {
        buf.putBit(1);
        buf.putBits(num_auxiliary_streams, 7);
        buf.putBit(1);
        buf.putBits(mae_groups.size(), 7);
        for (const auto& it : mae_groups) {
            buf.putBits(it.mae_group_id, 7);
            buf.putBit(it.is_in_main_stream);
            // Warning [1]: ISO/IEC 13818-1 says "if (thisIsMainStream == '0') {".
            // But this is meaningless since we are already in a branch where thisIsMainStream is always '1'.
            // Given the semantics of the following two fields, this is more likely "if (isInMainStream == '0')".
            if (!it.is_in_main_stream) {
                buf.putBit(it.is_in_ts);
                buf.putBits(it.auxiliary_stream_id, 7);
            }
        }
    }
    buf.putBytes(reserved);
}


//----------------------------------------------------------------------------
// Deserialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioMultiStreamDescriptor::deserializePayload(PSIBuffer& buf)
{
    this_is_main_stream = buf.getBool();
    buf.getBits(this_stream_id, 7);
    if (this_is_main_stream) {
        buf.skipBits(1);
        buf.getBits(num_auxiliary_streams, 7);
        buf.skipBits(1);
        const size_t count = buf.getBits<size_t>(7);
        for (size_t i = 0; i < count && !buf.error(); ++i) {
            Group gr;
            buf.getBits(gr.mae_group_id, 7);
            gr.is_in_main_stream = buf.getBool();
            // See warning [1] above.
            if (!gr.is_in_main_stream) {
                gr.is_in_ts = buf.getBool();
                buf.getBits(gr.auxiliary_stream_id, 7);
            }
            mae_groups.push_back(gr);
        }
    }
    buf.getBytes(reserved);
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioMultiStreamDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canRead()) {
        const bool main = buf.getBool();
        disp << margin << UString::Format(u"This is main stream: %s", {main}) << std::endl;
        disp << margin << UString::Format(u"This stream id: 0x%X (%<d)", {buf.getBits<uint8_t>(7)}) << std::endl;
        if (main && buf.canRead()) {
            buf.skipBits(1);
            disp << margin << UString::Format(u"Number of auxiliary streams: %d", {buf.getBits<uint8_t>(7)}) << std::endl;
            buf.skipBits(1);
            const size_t count = buf.getBits<size_t>(7);
            disp << margin << UString::Format(u"Number of mae groups: %d", {count}) << std::endl;
            for (size_t i = 0; i < count && buf.canRead(); ++i) {
                disp << margin << UString::Format(u"- MAE group id: 0x%X (%<d)", {buf.getBits<uint8_t>(7)}) << std::endl;
                const bool in_main = buf.getBool();
                disp << margin << UString::Format(u"  Is in main stream: %s", {in_main}) << std::endl;
                // See warning [1] above.
                if (!in_main && buf.canRead()) {
                    disp << margin << UString::Format(u"  Is in TS: %s", {buf.getBool()}) << std::endl;
                    disp << margin << UString::Format(u"  Auxiliary stream id: 0x%X (%<d)", {buf.getBits<uint8_t>(7)}) << std::endl;
                }
            }
        }
        disp.displayPrivateData(u"Reserved data", buf, NPOS, margin);
    }
}


//----------------------------------------------------------------------------
// XML serialization.
//----------------------------------------------------------------------------

void ts::MPEGH3DAudioMultiStreamDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    root->setBoolAttribute(u"this_is_main_stream", this_is_main_stream);
    root->setIntAttribute(u"this_stream_id", this_stream_id, true);
    if (this_is_main_stream) {
        root->setIntAttribute(u"num_auxiliary_streams", num_auxiliary_streams, false);
        for (const auto& it : mae_groups) {
            xml::Element* e = root->addElement(u"mae_group");
            e->setIntAttribute(u"mae_group_id", it.mae_group_id, true);
            e->setBoolAttribute(u"is_in_main_stream", it.is_in_main_stream);
            // See warning [1] above.
            if (!it.is_in_main_stream) {
                e->setBoolAttribute(u"is_in_ts", it.is_in_ts);
                e->setIntAttribute(u"auxiliary_stream_id", it.auxiliary_stream_id, true);
            }
        }
    }
    root->addHexaTextChild(u"reserved", reserved, true);
}


//----------------------------------------------------------------------------
// XML deserialization.
//----------------------------------------------------------------------------

bool ts::MPEGH3DAudioMultiStreamDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector xgroup;
    bool ok =
        element->getBoolAttribute(this_is_main_stream, u"this_is_main_stream", true) &&
        element->getIntAttribute(this_stream_id, u"this_stream_id", true, 0, 0, 0x7F) &&
        element->getIntAttribute(num_auxiliary_streams, u"num_auxiliary_streams", this_is_main_stream, 0, 0, 0x7F) &&
        element->getChildren(xgroup, u"mae_group", 0, this_is_main_stream ? 127 : 0) &&
        element->getHexaTextChild(reserved, u"reserved", false, 0, 255);

    for (auto it : xgroup) {
        Group gr;
        ok = it->getIntAttribute(gr.mae_group_id, u"mae_group_id", true, 0, 0, 0x7F) &&
             it->getBoolAttribute(gr.is_in_main_stream, u"is_in_main_stream", true) &&
             // See warning [1] above.
             it->getBoolAttribute(gr.is_in_ts, u"is_in_ts", !gr.is_in_main_stream) &&
             it->getIntAttribute(gr.auxiliary_stream_id, u"auxiliary_stream_id", !gr.is_in_main_stream, 0, 0, 0x7F);
        mae_groups.push_back(gr);
    }
    return ok;
}
