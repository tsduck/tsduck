//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMediaServiceKindDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"Media_service_kind_descriptor"
#define MY_CLASS ts::MediaServiceKindDescriptor
#define MY_DID ts::DID_MPEG_EXTENSION
#define MY_EDID ts::MPEG_EDID_MEDIA_SVC_KIND
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::ExtensionMPEG(MY_EDID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MediaServiceKindDescriptor::MediaServiceKindDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MediaServiceKindDescriptor::clearContent()
{
    media_service_kinds.clear();
}

ts::MediaServiceKindDescriptor::MediaServiceKindDescriptor(DuckContext& duck, const Descriptor& desc) :
    MediaServiceKindDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// This is an extension descriptor.
//----------------------------------------------------------------------------

ts::DID ts::MediaServiceKindDescriptor::extendedTag() const
{
    return MY_EDID;
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MediaServiceKindDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (auto msk : media_service_kinds) {
        bool identifier_flag = msk.ID_length_code.has_value() && msk.ID_type.has_value();
        buf.putBits(msk.media_description_flag, 1);
        buf.putBit(identifier_flag);
        buf.putBits(msk.language_media_service_type_pairs.size(), 3);
        buf.putBits(msk.media_type_idc, 2);
        buf.putBits(0xF, 1);
        if (identifier_flag) {
            buf.putBits(msk.ID_length_code.value(), 3);
            buf.putBits(msk.ID_type.value(), 13);
            if (msk.ID_length_code.value() == 7) {
                buf.putUInt8(msk.ID_len);
            }
            buf.putFixedUTF8(msk.media_ID_field, msk.ID_len);
        }
        for (auto language_pair : msk.language_media_service_type_pairs) {
            buf.putBits(language_pair.configuration_type, 2);
            buf.putBits(language_pair.media_service_types.size(), 3);
            buf.putBits(language_pair.lang_len_idc, 2);
            buf.putBits(0xF, 1);
            if (language_pair.lang_len_idc == 0) {
                buf.putUInt8(language_pair.lang_len);
            }
            buf.putFixedUTF8(language_pair.language_code, language_pair.lang_len);
            for (auto purpose : language_pair.media_service_types) {
                buf.putUInt8(purpose);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MediaServiceKindDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canReadBytes(1)) {
        media_service_kind_type newMSK;
        newMSK.media_description_flag = buf.getBit();
        bool _identifier_flag = buf.getBool();
        uint8_t _lang_pairs = buf.getBits<uint8_t>(3);
        newMSK.media_type_idc = buf.getBits<uint8_t>(2);
        buf.skipBits(1);
        if (_identifier_flag) {
            newMSK.ID_length_code = buf.getBits<uint8_t>(3);
            newMSK.ID_type = buf.getBits<uint16_t>(13);
            switch (newMSK.ID_length_code.value()) {
                case 0: newMSK.ID_len = 1; break;
                case 1: newMSK.ID_len = 2; break;
                case 2: newMSK.ID_len = 4; break;
                case 3: newMSK.ID_len = 8; break;
                case 4: newMSK.ID_len = 12; break;
                case 5: newMSK.ID_len = 16; break;
                case 6: newMSK.ID_len = 20; break;
                case 7: newMSK.ID_len = buf.getUInt8(); break;
                default: newMSK.ID_len = 0; break; // this should not happen, but keeps CodeQL happy
            }
            newMSK.media_ID_field = buf.getUTF8(newMSK.ID_len);
        }
        for (uint8_t j = 0; j < _lang_pairs; j++) {
            language_media_pair_type newLanguagePair;
            newLanguagePair.configuration_type = buf.getBits<uint8_t>(2);
            uint8_t _lang_purpose_cnt = buf.getBits<uint8_t>(3);
            newLanguagePair.lang_len_idc = buf.getBits<uint8_t>(2);
            buf.skipBits(1);
            switch (newLanguagePair.lang_len_idc) {
                case 0: newLanguagePair.lang_len = buf.getUInt8(); break;
                case 1: newLanguagePair.lang_len = 2; break;
                case 2: newLanguagePair.lang_len = 3; break;
                case 3: newLanguagePair.lang_len = 0; break; // 3 is reserved
                default: break;
            }
            newLanguagePair.language_code = buf.getUTF8(newLanguagePair.lang_len);
            for (uint8_t k = 0; k < _lang_purpose_cnt; k++) {
                newLanguagePair.media_service_types.push_back(buf.getUInt8());
            }
            newMSK.language_media_service_type_pairs.push_back(newLanguagePair);
        }
        media_service_kinds.push_back(newMSK);
    }
}


//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MediaServiceKindDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    uint16_t loop = 0;
    bool ok = true;
    while (ok && buf.canReadBytes(1)) {
        disp << margin << "[" << loop++ << "] " << DataName(MY_XML_NAME, u"media_description_flag", buf.getBit(), NamesFlags::VALUE | NamesFlags::DECIMAL);
        bool _identifier_flag = buf.getBool();
        uint8_t _lang_pairs = buf.getBits<uint8_t>(3);
        disp << ", media type: " << DataName(MY_XML_NAME, u"media_type", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL) << std::endl;
        buf.skipReservedBits(1);
        if (_identifier_flag) {
            uint8_t _ID_length_code = buf.getBits<uint8_t>(3), _ID_len = 0;
            disp << margin << " ID type: " << DataName(MY_XML_NAME, u"ID_type", buf.getBits<uint16_t>(13), NamesFlags::VALUE);
            switch (_ID_length_code) {
                case 0: _ID_len = 1; break;
                case 1: _ID_len = 2; break;
                case 2: _ID_len = 4; break;
                case 3: _ID_len = 8; break;
                case 4: _ID_len = 12; break;
                case 5: _ID_len = 16; break;
                case 6: _ID_len = 20; break;
                case 7: _ID_len = buf.getUInt8(); break;
                default: _ID_len = 0; ok = false; break; // this is an error
            }
            disp << ", media ID: " << ((_ID_len == 0) ? UString(u"!! length error!!") : buf.getUTF8(_ID_len)) << std::endl;
        }
        for (uint8_t i = 0; ok && i < _lang_pairs; i++) {
            disp << margin << "  language [" << int(i) << "] configuration: " << DataName(MY_XML_NAME, u"configuration_type", buf.getBits<uint8_t>(2), NamesFlags::VALUE | NamesFlags::DECIMAL);
            uint8_t _lang_purpose_cnt = buf.getBits<uint8_t>(3);
            uint8_t _lang_len_idc = buf.getBits<uint8_t>(2), _lang_len = 0;
            buf.skipReservedBits(1);
            switch (_lang_len_idc) {
                case 0: _lang_len = buf.getUInt8(); break;
                case 1: _lang_len = 2; break;
                case 2: _lang_len = 3; break;
                case 3: _lang_len = 0; ok = false; break;
                default: break;
            }
            disp << ", language: " << ((_lang_len == 0) ? UString(u"!! length error!!") : buf.getUTF8(_lang_len)) << std::endl;
            UStringVector purposes;
            for (uint8_t k=0; ok && k<_lang_purpose_cnt; k++) {
                purposes.push_back(DataName(MY_XML_NAME, u"purpose", buf.getUInt8(), NamesFlags::VALUE));
            }
            if (!purposes.empty()) {
                disp.displayVector(UString::Format(u"  Purpose%s:", { (purposes.size() > 1) ? "s" : "" }), purposes, margin, true, 2);
            }
        }

    }
}

//----------------------------------------------------------------------------
// Enumerations for XML.
//----------------------------------------------------------------------------

const ts::Enumeration ts::MediaServiceKindDescriptor::MediaDescriptionFlag({
    {u"self",      0},
    {u"associate", 1},
});
const ts::Enumeration ts::MediaServiceKindDescriptor::MediaType({
    {u"unknown",   0},
    {u"video",     1},
    {u"audio",     2},
    {u"text/data", 3},
    });

//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MediaServiceKindDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto msk : media_service_kinds) {
        ts::xml::Element* element = root->addElement(u"media_service_kind");
        element->setEnumAttribute(MediaDescriptionFlag, u"media_description", msk.media_description_flag);
        element->setEnumAttribute(MediaType, u"media_type", msk.media_type_idc);
        if (msk.ID_length_code.has_value() && msk.ID_type.has_value()) {
            element->setIntAttribute(u"ID_length_code", msk.ID_length_code.value());
            element->setIntAttribute(u"ID_type", msk.ID_type.value(), true);
            if (msk.ID_length_code.value() == 7) {
                element->setIntAttribute(u"ID_len", msk.ID_len);
            }
            element->setAttribute(u"media_ID", msk.media_ID_field);
        }
        for (auto language_pair : msk.language_media_service_type_pairs) {
            ts::xml::Element* languagePair = element->addElement(u"language_media_pair");
            languagePair->setIntAttribute(u"configuration_type", language_pair.configuration_type);
            languagePair->setIntAttribute(u"lang_len_idc", language_pair.lang_len_idc);
            if (language_pair.lang_len_idc == 0) {
                languagePair->setIntAttribute(u"lang_len", language_pair.lang_len);
            }
            languagePair->setAttribute(u"BCP47_language_code", language_pair.language_code);
            for (auto purpose : language_pair.media_service_types) {
                ts::xml::Element* serviceType = languagePair->addElement(u"media_service_type");
                serviceType->setIntAttribute(u"purpose", purpose, true);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MediaServiceKindDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector children;
    bool ok = element->getChildren(children, u"media_service_kind");

    if (ok) {
        for (size_t i=0; ok && i<children.size(); i++) {
            media_service_kind_type newMSK;
            int mdf=0, mt=0;
            ok = children[i]->getEnumAttribute(mdf, MediaDescriptionFlag, u"media_description", true) &&
                 children[i]->getEnumAttribute(mt, MediaType, u"media_type", true) &&
                 children[i]->getOptionalIntAttribute(newMSK.ID_length_code, u"ID_length_code", 0, 0x7) &&
                 children[i]->getOptionalIntAttribute(newMSK.ID_type, u"ID_type", 0, 0x1FFF);
            newMSK.media_description_flag = uint8_t(mdf);
            newMSK.media_type_idc = uint8_t(mt);

            if (ok) {
                if (newMSK.ID_length_code.has_value()) {
                    switch (newMSK.ID_length_code.value()) {
                        case 0: newMSK.ID_len = 1; break;
                        case 1: newMSK.ID_len = 2; break;
                        case 2: newMSK.ID_len = 4; break;
                        case 3: newMSK.ID_len = 8; break;
                        case 4: newMSK.ID_len = 12; break;
                        case 5: newMSK.ID_len = 16; break;
                        case 6: newMSK.ID_len = 20; break;
                        case 7:
                            ok = children[i]->getIntAttribute(newMSK.ID_len, u"ID_len");
                            break;
                        default:
                            ok = false;
                            break;
                    }
                }
                if (ok && (newMSK.ID_len != 0)) {
                    ok &= children[i]->getAttribute(newMSK.media_ID_field, u"media_ID", true, UString('*', newMSK.ID_len), newMSK.ID_len, newMSK.ID_len);
                }
            }
            xml::ElementVector mediaPairs;
            ok &= children[i]->getChildren(mediaPairs, u"language_media_pair", 0, 0x7);
            for (size_t j = 0; ok && j < mediaPairs.size(); j++) {
                language_media_pair_type newLanguagePair;
                ok = mediaPairs[j]->getIntAttribute(newLanguagePair.configuration_type, u"configuration_type", true, 0, 0, 3) &&
                     mediaPairs[j]->getIntAttribute(newLanguagePair.lang_len_idc, u"lang_len_idc", true, 0, 0, 3);
                if (ok) {
                    switch (newLanguagePair.lang_len_idc) {
                    case 0:
                        mediaPairs[j]->getIntAttribute(newLanguagePair.lang_len, u"lang_len", true, 0);
                        break;
                    case 1: newLanguagePair.lang_len = 2; break;
                    case 2: newLanguagePair.lang_len = 3; break;
                    case 3:
                        mediaPairs[j]->report().error(u"'3' is a reserved value for @lang_len_idc in <%s>, line %d", { element->name(), element->lineNumber() });
                        ok = false;
                        break;
                    default:
                        ok = false;
                    }
                }
                ok &= mediaPairs[j]->getAttribute(newLanguagePair.language_code, u"BCP47_language_code", true, u"");
                if (newLanguagePair.language_code.length() != newLanguagePair.lang_len) {
                    mediaPairs[j]->report().error(u"specified length (%d) does not match @IETF_BCP_47_language_code length (%d) in <%s>, line %d", { newLanguagePair.lang_len, newLanguagePair.language_code.length(), element->name(), element->lineNumber() });
                    ok = false;
                }
                xml::ElementVector mediaServiceTypes;
                ok &= mediaPairs[j]->getChildren(mediaServiceTypes, u"media_service_type", 0, 7);
                for (size_t k = 0; ok && k < mediaServiceTypes.size(); k++) {
                    uint8_t mstp;
                    ok = mediaServiceTypes[k]->getIntAttribute(mstp, u"purpose", true, 0, 0x00);
                    if (ok) {
                        newLanguagePair.media_service_types.push_back(mstp);
                    }
                }
                newMSK.language_media_service_type_pairs.push_back(newLanguagePair);
            }
            media_service_kinds.push_back(newMSK);
        }
    }
    return ok;
}
