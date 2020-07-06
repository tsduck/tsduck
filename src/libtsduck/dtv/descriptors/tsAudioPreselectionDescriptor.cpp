//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsAudioPreselectionDescriptor.h"
#include "tsDescriptor.h"
#include "tsNames.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

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
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0),
    entries()
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

ts::AudioPreselectionDescriptor::PreSelection::PreSelection() :
    preselection_id(0),
    audio_rendering_indication(0),
    audio_description(false),
    spoken_subtitles(false),
    dialogue_enhancement(false),
    interactivity_enabled(false),
    ISO_639_language_code(),
    message_id(),
    aux_component_tags(),
    future_extension()
{
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

void ts::AudioPreselectionDescriptor::serialize(DuckContext& duck, Descriptor& desc) const
{
    if (!hasValidSizes()) {
        desc.invalidate();
        return;
    }

    ByteBlockPtr bbp(serializeStart());
    bbp->appendUInt8(MY_EDID);
    bbp->appendUInt8(uint8_t(entries.size() << 3));
    for (auto it = entries.begin(); it != entries.end(); ++ it) {
        bbp->appendUInt8(uint8_t(it->preselection_id << 3) | (it->audio_rendering_indication & 0x07));
        bbp->appendUInt8((it->audio_description ? 0x80 : 0x00) |
                         (it->spoken_subtitles ? 0x40 : 0x00) |
                         (it->dialogue_enhancement ? 0x20 : 0x00) |
                         (it->interactivity_enabled ? 0x10 : 0x00) |
                         (it->ISO_639_language_code.empty() ? 0x00 : 0x08) |
                         (it->message_id.set() ? 0x04 : 0x00) |
                         (it->aux_component_tags.empty() ? 0x00 : 0x02) |
                         (it->future_extension.empty() ? 0x00 : 0x01));

        if (!it->ISO_639_language_code.empty() && !SerializeLanguageCode(*bbp, it->ISO_639_language_code)) {
            desc.invalidate();
            return;
        }
        if (it->message_id.set()) {
            bbp->appendUInt8(it->message_id.value());
        }
        if (!it->aux_component_tags.empty()) {
            bbp->appendUInt8(uint8_t(it->aux_component_tags.size() << 5));
            bbp->append(it->aux_component_tags);
        }
        if (!it->future_extension.empty()) {
            bbp->appendUInt8(uint8_t(it->future_extension.size() & 0x1F));
            bbp->append(it->future_extension);
        }
    }
    serializeEnd(desc, bbp);
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::AudioPreselectionDescriptor::deserialize(DuckContext& duck, const Descriptor& desc)
{
    const uint8_t* data = desc.payload();
    size_t size = desc.payloadSize();

    entries.clear();
    _is_valid = desc.isValid() && desc.tag() == tag() && size >= 2 && data[0] == MY_EDID;

    if (_is_valid) {
        size_t numEntries = data[1] >> 3;
        data += 2;
        size -= 2;

        while (_is_valid && numEntries > 0 && size >= 2) {

            PreSelection sel;
            sel.preselection_id = data[0] >> 3;
            sel.audio_rendering_indication = data[0] & 0x07;
            sel.audio_description = (data[1] & 0x80) != 0;
            sel.spoken_subtitles = (data[1] & 0x40) != 0;
            sel.dialogue_enhancement = (data[1] & 0x20) != 0;
            sel.interactivity_enabled = (data[1] & 0x10) != 0;
            const bool hasLanguage = (data[1] & 0x08) != 0;
            const bool hasLabel = (data[1] & 0x04) != 0;
            const bool hasMultiStream = (data[1] & 0x02) != 0;
            const bool hasExtension = (data[1] & 0x01) != 0;
            data += 2;
            size -= 2;

            if (hasLanguage) {
                _is_valid = size >= 3;
                if (_is_valid) {
                    sel.ISO_639_language_code = DeserializeLanguageCode(data);
                    data += 3;
                    size -= 3;
                }
            }
            if (_is_valid && hasLabel) {
                _is_valid = size >= 1;
                if (_is_valid) {
                    sel.message_id = data[0];
                    data += 1;
                    size -= 1;
                }
            }
            if (_is_valid && hasMultiStream) {
                _is_valid = size >= 1;
                const size_t num = _is_valid ? (data[0] >> 5) : 0;
                _is_valid = _is_valid && size >= 1 + num;
                if (_is_valid) {
                    sel.aux_component_tags.copy(data + 1, num);
                    data += 1 + num;
                    size -= 1 + num;
                }
            }
            if (_is_valid && hasExtension) {
                _is_valid = size >= 1;
                const size_t len = _is_valid ? (data[0] & 0x1F) : 0;
                _is_valid = _is_valid && size >= 1 + len;
                if (_is_valid) {
                    sel.future_extension.copy(data + 1, len);
                    data += 1 + len;
                    size -= 1 + len;
                }
            }

            if (_is_valid) {
                entries.push_back(sel);
                numEntries--;
            }
        }

        _is_valid = _is_valid && numEntries == 0 && size == 0;
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::AudioPreselectionDescriptor::DisplayDescriptor(TablesDisplay& display, DID did, const uint8_t* data, size_t size, int indent, TID tid, PDS pds)
{
    // Important: With extension descriptors, the DisplayDescriptor() function is called
    // with extension payload. Meaning that data points after descriptor_tag_extension.
    // See ts::TablesDisplay::displayDescriptorData()

    DuckContext& duck(display.duck());
    std::ostream& strm(duck.out());
    const std::string margin(indent, ' ');

    if (size >= 1) {
        size_t numEntries = data[0] >> 3;
        data += 1;
        size -= 1;

        for (bool valid = true; valid && numEntries > 0 && size >= 2; numEntries--) {

            strm << margin << UString::Format(u"- Preselection id: %d", {data[0] >> 3}) << std::endl
                 << margin << "  Audio rendering indication: " << NameFromSection(u"AudioPreselectionRendering", data[0] & 0x07, names::DECIMAL_FIRST) << std::endl
                 << margin << "  Audio description: " << UString::YesNo((data[1] & 0x80) != 0) << std::endl
                 << margin << "  Spoken subtitles: " << UString::YesNo((data[1] & 0x40) != 0) << std::endl
                 << margin << "  Dialogue enhancement: " << UString::YesNo((data[1] & 0x20) != 0) << std::endl
                 << margin << "  Interactivity enabled: " << UString::YesNo((data[1] & 0x10) != 0) << std::endl;

            const bool hasLanguage = (data[1] & 0x08) != 0;
            const bool hasLabel = (data[1] & 0x04) != 0;
            const bool hasMultiStream = (data[1] & 0x02) != 0;
            const bool hasExtension = (data[1] & 0x01) != 0;
            data += 2;
            size -= 2;

            if (hasLanguage) {
                valid = size >= 3;
                if (valid) {
                    strm << margin << "  Language code: \"" << DeserializeLanguageCode(data) << '"' << std::endl;
                    data += 3;
                    size -= 3;
                }
            }
            if (valid && hasLabel) {
                valid = size >= 1;
                if (valid) {
                    strm << margin << UString::Format(u"  Text label / message id: 0x%0X (%d)", {data[0], data[0]}) << std::endl;
                    data += 1;
                    size -= 1;
                }
            }
            if (valid && hasMultiStream) {
                valid = size >= 1;
                const size_t num = valid ? (data[0] >> 5) : 0;
                valid = valid && size >= 1 + num;
                if (valid) {
                    strm << margin << UString::Format(u"  Multi stream info: %d aux components", {num}) << std::endl;
                    for (size_t i = 1; i <= num; ++i) {
                        strm << margin << UString::Format(u"    Component tag: 0x%0X (%d)", {data[i], data[i]}) << std::endl;
                    }
                    data += 1 + num;
                    size -= 1 + num;
                }
            }
            if (valid && hasExtension) {
                valid = size >= 1;
                const size_t len = valid ? (data[0] & 0x1F) : 0;
                valid = valid && size >= 1 + len;
                if (valid) {
                    display.displayPrivateData(u"Future extension", data + 1, len, indent + 2);
                    data += 1 + len;
                    size -= 1 + len;
                }
            }
        }
    }

    display.displayExtraData(data, size, indent);
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

    for (size_t i = 0; _is_valid && i < children.size(); ++i) {
        PreSelection sel;
        xml::ElementVector msi;
        xml::ElementVector comps;
        ok = children[i]->getIntAttribute<uint8_t>(sel.preselection_id, u"preselection_id", true, 0, 0x00, 0x1F) &&
             children[i]->getIntAttribute<uint8_t>(sel.audio_rendering_indication, u"audio_rendering_indication", true, 0, 0x00, 0x07) &&
             children[i]->getBoolAttribute(sel.audio_description, u"audio_description", false, false) &&
             children[i]->getBoolAttribute(sel.spoken_subtitles, u"spoken_subtitles", false, false) &&
             children[i]->getBoolAttribute(sel.dialogue_enhancement, u"dialogue_enhancement", false, false) &&
             children[i]->getBoolAttribute(sel.interactivity_enabled, u"interactivity_enabled", false, false) &&
             children[i]->getAttribute(sel.ISO_639_language_code, u"ISO_639_language_code", false, u"", 3, 3) &&
             children[i]->getOptionalIntAttribute<uint8_t>(sel.message_id, u"message_id") &&
             children[i]->getChildren(msi, u"multi_stream_info", 0, 1) &&
             (msi.empty() || msi.front()->getChildren(comps, u"component", 0, 0x07)) &&
             children[i]->getHexaTextChild(sel.future_extension, u"future_extension", false, 0, 0x1F);

        for (size_t i2 = 0; ok && i2 < comps.size(); ++i2) {
            uint8_t t = 0;
            ok = comps[i2]->getIntAttribute<uint8_t>(t, u"tag", true);
            sel.aux_component_tags.push_back(t);
        }
        entries.push_back(sel);
    }
    return ok && hasValidSizes();
}
