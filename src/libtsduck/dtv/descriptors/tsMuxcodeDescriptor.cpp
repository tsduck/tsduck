//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2022-2023, Paul Higgs
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsMuxcodeDescriptor.h"
#include "tsDescriptor.h"
#include "tsTablesDisplay.h"
#include "tsPSIRepository.h"
#include "tsPSIBuffer.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"

#define MY_XML_NAME u"MuxCode_descriptor"
#define MY_CLASS ts::MuxCodeDescriptor
#define MY_DID ts::DID_MUXCODE
#define MY_STD ts::Standards::MPEG

TS_REGISTER_DESCRIPTOR(MY_CLASS, ts::EDID::Standard(MY_DID), MY_XML_NAME, MY_CLASS::DisplayDescriptor);

auto constexpr MAX_SUBSTRUCTURES = 0xFF;    // 8 bits for the substructureCount
auto constexpr MAX_SLOTS = 0x1F;            // 5 bits for the slotCount


//----------------------------------------------------------------------------
// Constructors
//----------------------------------------------------------------------------

ts::MuxCodeDescriptor::MuxCodeDescriptor() :
    AbstractDescriptor(MY_DID, MY_XML_NAME, MY_STD, 0)
{
}

void ts::MuxCodeDescriptor::clearContent()
{
    MuxCodeTableEntry.clear();
}

ts::MuxCodeDescriptor::MuxCodeDescriptor(DuckContext& duck, const Descriptor& desc) :
    MuxCodeDescriptor()
{
    deserialize(duck, desc);
}


//----------------------------------------------------------------------------
// Serialization
//----------------------------------------------------------------------------

void ts::MuxCodeDescriptor::serializePayload(PSIBuffer& buf) const
{
    for (auto it : MuxCodeTableEntry) {
        uint8_t _length = 2;  // 4 bit MuxCode, 4 bit version, 8 bit substructureCount
        for (auto it2 : it.substructure) {
            _length += 1 + // 5 bit slotCount, 3 bit repetitionCode
                (2 * uint8_t(std::min(it2.m4MuxChannel.size(), it2.numberOfBytes.size())));
        }
        buf.putUInt8(_length);
        buf.putBits(it.MuxCode, 4);
        buf.putBits(it.version, 4);
        buf.putUInt8(uint8_t(it.substructure.size()));
        for (auto it2 : it.substructure) {
            uint8_t slotCount = uint8_t(std::min(it2.m4MuxChannel.size(), it2.numberOfBytes.size()));
            buf.putBits(slotCount, 5);
            buf.putBits(it2.repititionCount, 3);
            for (uint8_t i = 0; i < slotCount; i++) {
                buf.putUInt8(it2.m4MuxChannel[i]);
                buf.putUInt8(it2.numberOfBytes[i]);
            }
        }
    }
}


//----------------------------------------------------------------------------
// Deserialization
//----------------------------------------------------------------------------

void ts::MuxCodeDescriptor::deserializePayload(PSIBuffer& buf)
{
    while (buf.canReadBytes(3)) {
        uint8_t length = buf.getUInt8();
        if (buf.canReadBytes(length)) {
            MuxCodeTableEntry_type newMuxCodeEntry;
            buf.getBits(newMuxCodeEntry.MuxCode, 4);
            buf.getBits(newMuxCodeEntry.version, 4);
            uint8_t substructreCount = buf.getUInt8();
            for (uint8_t i = 0; i < substructreCount; i++) {
                substructure_type newSubstructure;
                uint8_t slotCount;
                buf.getBits(slotCount, 5);
                buf.getBits(newSubstructure.repititionCount, 3);
                for (uint8_t k = 0; k < slotCount; k++) {
                    uint8_t m4MuxChannel = buf.getUInt8();
                    newSubstructure.m4MuxChannel.push_back(m4MuxChannel);
                    uint8_t numberOfBytes = buf.getUInt8();
                    newSubstructure.numberOfBytes.push_back(numberOfBytes);
                }
                newMuxCodeEntry.substructure.push_back(newSubstructure);
            }
            MuxCodeTableEntry.push_back(newMuxCodeEntry);
        }
    }
}

//----------------------------------------------------------------------------
// Static method to display a descriptor.
//----------------------------------------------------------------------------

void ts::MuxCodeDescriptor::DisplayDescriptor(TablesDisplay& disp, PSIBuffer& buf, const UString& margin, DID did, TID tid, PDS pds)
{
    uint8_t MuxCodeIndex = 0;
    while (buf.canReadBytes(3)) {
        buf.skipBits(8);  // length
        disp << margin << "index[" << int(MuxCodeIndex++) << "] MuxCode: " << buf.getBits<uint16_t>(4);
        disp << ", version: " << buf.getBits<uint16_t>(4) << std::endl;
        uint8_t _substructureCount = buf.getUInt8();
        for (uint8_t i = 0; i < _substructureCount; i++) {
            uint8_t _slotCount = buf.getBits<uint8_t>(5);
            disp << margin << " substructure[" << int(i) << "], repetition count: " << buf.getBits<uint16_t>(3) << std::endl;
            for (uint8_t k = 0; k < _slotCount; k++) {
                disp << margin << "  M4 mux channel: " << int(buf.getUInt8());
                disp << ", byte count: " << int(buf.getUInt8()) << std::endl;
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML serialization
//----------------------------------------------------------------------------

void ts::MuxCodeDescriptor::buildXML(DuckContext& duck, xml::Element* root) const
{
    for (auto it : MuxCodeTableEntry) {
        ts::xml::Element* _entry = root->addElement(u"MuxCodeEntry");
        _entry->setIntAttribute(u"MuxCode", it.MuxCode);
        _entry->setIntAttribute(u"version", it.version);

        for (auto it2 : it.substructure) {
            ts::xml::Element* _substructure = _entry->addElement(u"substructure");
            _substructure->setIntAttribute(u"repetitionCount", it2.repititionCount);
            uint8_t slotCount = uint8_t(std::min(it2.m4MuxChannel.size(), it2.numberOfBytes.size()));
            for (uint8_t k = 0; k < slotCount; k++) {
                ts::xml::Element* _slot = _substructure->addElement(u"slot");
                _slot->setIntAttribute(u"m4MuxChannel", it2.m4MuxChannel[k]);
                _slot->setIntAttribute(u"numberOfBytes", it2.numberOfBytes[k]);
            }
        }
    }
}


//----------------------------------------------------------------------------
// XML deserialization
//----------------------------------------------------------------------------

bool ts::MuxCodeDescriptor::analyzeXML(DuckContext& duck, const xml::Element* element)
{
    xml::ElementVector MuxCodeEntries;
    bool ok = element->getChildren(MuxCodeEntries, u"MuxCodeEntry");

    for (size_t i = 0; ok && i < MuxCodeEntries.size(); ++i) {
        MuxCodeTableEntry_type MuxCodeEntry;
        ok &= MuxCodeEntries[i]->getIntAttribute(MuxCodeEntry.MuxCode, u"MuxCode", true, 0, 0, 0x0F);
        ok &= MuxCodeEntries[i]->getIntAttribute(MuxCodeEntry.version, u"version", true, 0, 0, 0x0F);

        xml::ElementVector subStructures;
        ok &= MuxCodeEntries[i]->getChildren(subStructures, u"substructure");
        if (subStructures.size() > MAX_SUBSTRUCTURES) {
            element->report().error(u"only %d <substructure> elements are permitted [<%s>, line %d]", { MAX_SUBSTRUCTURES, element->name(), element->lineNumber() });
            ok = false;
        }
        for (size_t j = 0; ok && j < subStructures.size(); ++j) {
            substructure_type _substructure;

            ok &= subStructures[j]->getIntAttribute(_substructure.repititionCount, u"repetitionCount", true, 0, 0, 0x07);

            if ((_substructure.repititionCount == 0) && j != (subStructures.size() - 1)) {
                // repetitionCount of zero is only permitted in the last substructire (ISO/IEC 14496-1 clause 7.4.2.5.2)
                element->report().error(u"repetitionCount=='%d' is only valid the last <substructure> [<%s>, line %d]", { _substructure.repititionCount, element->name(), element->lineNumber() });
                ok = false;
            }
            xml::ElementVector slots;
            ok &= subStructures[j]->getChildren(slots, u"slot");
            if (slots.size() > MAX_SLOTS) {
                element->report().error(u"only %d <slot> elements are permitted [<%s>, line %d]", { MAX_SLOTS, element->name(), element->lineNumber() });
                ok = false;
            }
            for (size_t k = 0; ok && k < slots.size(); k++) {
                uint32_t _tmp;
                ok &= slots[k]->getIntAttribute(_tmp, u"m4MuxChannel", true, 0, 0, 0xFF);
                _substructure.m4MuxChannel.push_back(uint8_t(_tmp));
                ok &= slots[k]->getIntAttribute(_tmp, u"numberOfBytes", true, 0, 0, 0xFF);
                _substructure.numberOfBytes.push_back(uint8_t(_tmp));
            }
            MuxCodeEntry.substructure.push_back(_substructure);

        }
        MuxCodeTableEntry.push_back(MuxCodeEntry);
    }
    return ok;
}
