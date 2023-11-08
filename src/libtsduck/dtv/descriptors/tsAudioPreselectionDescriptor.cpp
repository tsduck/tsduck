//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsAudioPreselectionDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"audio_preselection_descriptor"
#define MY_CLASS ts::AudioPreselectionDescriptor
#define MY_DID ts::DID_DVB_EXTENSION
#define MY_EDID ts::EDID_AUDIO_PRESELECT
#define MY_STD ts::Standards::DVB

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionDVB(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::AudioPreselectionDescriptor::AudioPreselectionDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

ts::AudioPreselectionDescriptor::AudioPreselectionDescriptor(DuckContext& duck, const Descriptor& desc) :
    AudioPreselectionDescriptor()
{
    deserialize(duck, desc);
}

void ts::AudioPreselectionDescriptor::clearContent()
{
    entries.clear();
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::AudioPreselectionDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Check if internal data sizes are valid.
//----------------------------------------------------------------------------

bool ts::AudioPreselectionDescriptor::hasValidSizes() const
{
    // Number of preselections uses 5 bits.
    bool ok = entries.size() <= 0x1F;
    // Check data sizes in all entries.
    for (auto it = entries.begin(); ok && it != entries.end(); ++ it) {
        ok = (it->ISO_639_language_code.empty() || it->ISO_639_language_code.size() == 3) &&
            it->aux_component_tags.size() <= 0x07 &&
            it->future_extension.size() <= 0x1F;
    }
    return ok;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::AudioPreselectionDescriptor::serializePayload(PSIBuffer& buf) const
{
    buf.putBits(entries.size(), 5);
    buf.putBits(0x00, 3); // reserved_zero_future_use
    for (auto it = entries.begin(); it != entries.end(); ++ it) {
        buf.putBits(it->preselection_id, 5);
        buf.putBits(it->audio_rendering_indication, 3);
        buf.putBit(it->audio_description);
        buf.putBit(it->spoken_subtitles);
        buf.putBit(it->dialogue_enhancement);
        buf.putBit(it->interactivity_enabled);
        buf.putBit(!it->ISO_639_language_code.empty());
        buf.putBit(it->message_id.has_value());
        buf.putBit(!it->aux_component_tags.empty());
        buf.putBit(!it->future_extension.empty());
        if (!it->ISO_639_language_code.empty()) {
            buf.putLanguageCode(it->ISO_639_language_code);
        }
        if (it->message_id.has_value()) {
            buf.putUInt8(it->message_id.value());
        }
        if (!it->aux_component_tags.empty()) {
            buf.putBits(it->aux_component_tags.size(), 3);
            buf.putBits(0x00, 5); // reserved_zero_future_use
            buf.putBytes(it->aux_component_tags);
        }
        if (!it->future_extension.empty()) {
            buf.putBits(0x00, 3); // reserved_zero_future_use
            buf.putBits(it->future_extension.size(), 5);
            buf.putBytes(it->future_extension);
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AudioPreselectionDescriptor::deserializePayload(PSIBuffer& buf)
{
    size_t numEntries = buf.getBits<size_t>(5);
    buf.skipBits(3);

    while (!buf.error() && numEntries > 0) {
        PreSelection sel;
        buf.getBits(sel.preselection_id, 5);
        buf.getBits(sel.audio_rendering_indication, 3);
        sel.audio_description = buf.getBool();
        sel.spoken_subtitles = buf.getBool();
        sel.dialogue_enhancement = buf.getBool();
        sel.interactivity_enabled = buf.getBool();
        const bool hasLanguage = buf.getBool();
        const bool hasLabel = buf.getBool();
        const bool hasMultiStream = buf.getBool();
        const bool hasExtension = buf.getBool();

        if (hasLanguage) {
            buf.getLanguageCode(sel.ISO_639_language_code);
        }
        if (hasLabel) {
            sel.message_id = buf.getUInt8();
        }
        if (hasMultiStream) {
            const size_t num = buf.getBits<size_t>(3);
            buf.skipBits(5);
            buf.getBytes(sel.aux_component_tags, num);
        }
        if (hasExtension) {
            buf.skipBits(3);
            const size_t len = buf.getBits<size_t>(5);
            buf.getBytes(sel.future_extension, len);
        }
        entries.push_back(sel);
        numEntries--;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AudioPreselectionDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    if (buf.canReadBytes(1)) {
        const size_t count = buf.getBits<size_t>(5);
        buf.skipBits(3);
        for (size_t i = 0; buf.canReadBytes(2) && i < count; ++i) {
            disp << margin << UString::Format(u"- Preselection id: %d", {buf.getBits<uint8_t>(5)}) << std::endl;
            disp << margin << "  Audio rendering indication: " << DataName(MY_XML_NAME, u"Rendering", buf.getBits<uint8_t>(3), NamesFlags::DECIMAL_FIRST) << std::endl;
            disp << margin << "  Audio description: " << UString::YesNo(buf.getBool()) << std::endl;
            disp << margin << "  Spoken subtitles: " << UString::YesNo(buf.getBool()) << std::endl;
            disp << margin << "  Dialogue enhancement: " << UString::YesNo(buf.getBool()) << std::endl;
            disp << margin << "  Interactivity enabled: " << UString::YesNo(buf.getBool()) << std::endl;

            const bool hasLanguage = buf.getBool();
            const bool hasLabel = buf.getBool();
            const bool hasMultiStream = buf.getBool();
            const bool hasExtension = buf.getBool();

            if (hasLanguage && buf.canReadBytes(3)) {
                disp << margin << "  Language code: \"" << buf.getLanguageCode() << '"' << std::endl;
            }
            if (hasLabel && buf.canReadBytes(1)) {
                disp << margin << UString::Format(u"  Text label / message id: 0x%0X (%<d)", {buf.getUInt8()}) << std::endl;
            }
            if (hasMultiStream && buf.canReadBytes(1)) {
                const size_t num = buf.getBits<size_t>(3);
                buf.skipBits(5);
                disp << margin << UString::Format(u"  Multi stream info: %d aux components", {num}) << std::endl;
                for (size_t n = 1; n <= num; ++n) {
                    disp << margin << UString::Format(u"    Component tag: 0x%0X (%<d)", {buf.getUInt8()}) << std::endl;
                }
            }
            if (hasExtension && buf.canReadBytes(1)) {
                buf.skipBits(3);
                const size_t len = buf.getBits<size_t>(5);
                disp.displayPrivateData(u"Future extension", buf, len, margin + u"  ");
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::AudioPreselectionDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it = entries.begin(); it != entries.end(); ++ it) {
        xml::Element* e = root->addElement(u"preselection");
        e->setIntAttribute(u"preselection_id", it->preselection_id, false);
        e->setIntAttribute(u"audio_rendering_indication", it->audio_rendering_indication, false);
        e->setBoolAttribute(u"audio_description", it->audio_description);
        e->setBoolAttribute(u"spoken_subtitles", it->spoken_subtitles);
        e->setBoolAttribute(u"dialogue_enhancement", it->dialogue_enhancement);
        e->setBoolAttribute(u"interactivity_enabled", it->interactivity_enabled);
        if (it->ISO_639_language_code.size() == 3) {
            e->setAttribute(u"ISO_639_language_code", it->ISO_639_language_code);
        }
        e->setOptionalIntAttribute(u"message_id", it->message_id, true);
        if (!it->aux_component_tags.empty()) {
            xml::Element* info = e->addElement(u"multi_stream_info");
            for (size_t i = 0; i < it->aux_component_tags.size(); ++i) {
                info->addElement(u"component")->setIntAttribute(u"tag", it->aux_component_tags[i], true);
            }
        }
        if (!it->future_extension.empty()) {
            e->addHexaTextChild(u"future_extension", it->future_extension);
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::AudioPreselectionDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"preselection");

    for (size_t i = 0; ok && i < children.size(); ++i) {
        PreSelection sel;
        xml::ElementVector msi;
        xml::ElementVector comps;
        ok = children[i]->getIntAttribute(sel.preselection_id, u"preselection_id", true, 0, 0x00, 0x1F) &&
             children[i]->getIntAttribute(sel.audio_rendering_indication, u"audio_rendering_indication", true, 0, 0x00, 0x07) &&
             children[i]->getBoolAttribute(sel.audio_description, u"audio_description", false, false) &&
             children[i]->getBoolAttribute(sel.spoken_subtitles, u"spoken_subtitles", false, false) &&
             children[i]->getBoolAttribute(sel.dialogue_enhancement, u"dialogue_enhancement", false, false) &&
             children[i]->getBoolAttribute(sel.interactivity_enabled, u"interactivity_enabled", false, false) &&
             children[i]->getAttribute(sel.ISO_639_language_code, u"ISO_639_language_code", false, u"", 3, 3) &&
             children[i]->getOptionalIntAttribute(sel.message_id, u"message_id") &&
             children[i]->getChildren(msi, u"multi_stream_info", 0, 1) &&
             (msi.empty() || msi.front()->getChildren(comps, u"component", 0, 0x07)) &&
             children[i]->getHexaTextChild(sel.future_extension, u"future_extension", false, 0, 0x1F);

        for (size_t i2 = 0; ok && i2 < comps.size(); ++i2) {
            uint8_t t = 0;
            ok = comps[i2]->getIntAttribute(t, u"tag", true);
            sel.aux_component_tags.push_back(t);
        }
        entries.push_back(sel);
    }
    return ok && hasValidSizes();
}
