//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2019, Thierry Lelegard
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------

#include "tsHFBand.h"
#include "tsDuckConfigFile.h"
#include "tsGuard.h"
#include "tsxmlDocument.h"
#include "tsxmlElement.h"
TSDUCK_SOURCE;

// An enumeration object for BandType.
const ts::Enumeration ts::HFBand::BandTypeEnum({
    {u"VHF", ts::HFBand::VHF},
    {u"UHF", ts::HFBand::UHF},
});


//----------------------------------------------------------------------------
// Constructors and destructors
//----------------------------------------------------------------------------

ts::HFBand::HFBand(BandType t) :
    _type(t),
    _channel_count(0),
    _regions(),
    _channels()
{
}

ts::HFBand::ChannelsRange::ChannelsRange() :
    first_channel(0),
    last_channel(0),
    base_frequency(0),
    channel_width(0),
    first_offset(0),
    last_offset(0),
    offset_width(0)
{
}


//----------------------------------------------------------------------------
// Factory static method.
//----------------------------------------------------------------------------

ts::HFBandPtr ts::HFBand::Factory(const UString& region, BandType type, Report& report)
{
    HFBandRepository* repo = HFBandRepository::Instance();
    repo->load(report);
    return repo->get(type, region, report);
}


ts::UString ts::HFBand::DefaultRegion(Report& report)
{
    HFBandRepository* repo = HFBandRepository::Instance();
    repo->load(report);
    return repo->defaultRegion();
}


//----------------------------------------------------------------------------
// Get the range of channels for a given channel number.
//----------------------------------------------------------------------------

ts::HFBand::ChannelsRangeList::const_iterator ts::HFBand::getRange(uint32_t channel) const
{
    for (ChannelsRangeList::const_iterator it = _channels.begin(); it != _channels.end(); ++it) {
        if (channel < it->first_channel) {
            return _channels.end();
        }
        else if (channel <= it->last_channel) {
            return it;
        }
    }
    return _channels.end();
}


//----------------------------------------------------------------------------
// Get channel attributes.
//----------------------------------------------------------------------------

uint32_t ts::HFBand::nextChannel(uint32_t channel) const
{
    ChannelsRangeList::const_iterator it(getRange(channel));
    if (it == _channels.end()) {
        // Not in any range.
        return 0;
    }
    else if (channel < it->last_channel) {
        // Not the last in range.
        return channel + 1;
    }
    else if (++it == _channels.end()) {
        // Last of last range.
        return 0;
    }
    else {
        // Get first of next range.
        return it->first_channel;
    }
}

uint32_t ts::HFBand::previousChannel(uint32_t channel) const
{
    ChannelsRangeList::const_iterator it(getRange(channel));
    if (it == _channels.end()) {
        // Not in any range.
        return 0;
    }
    else if (channel > it->first_channel) {
        // Not the first in range.
        return channel - 1;
    }
    else if (it == _channels.begin()) {
        // First of first range.
        return 0;
    }
    else {
        // Get last of previous range.
        return (--it)->last_channel;
    }
}

uint64_t ts::HFBand::frequency(uint32_t channel, int32_t offset) const
{
    ChannelsRangeList::const_iterator it(getRange(channel));
    if (it == _channels.end()) {
        return 0;
    }
    else {
        return it->base_frequency + (channel - it->first_channel) * it->channel_width + int64_t(offset * int64_t(it->offset_width));
    }
}

uint64_t ts::HFBand::bandWidth(uint32_t channel) const
{
    ChannelsRangeList::const_iterator it(getRange(channel));
    return it == _channels.end() ? 0 : it->channel_width;
}

uint64_t ts::HFBand::offsetWidth(uint32_t channel) const
{
    ChannelsRangeList::const_iterator it(getRange(channel));
    return it == _channels.end() ? 0 : it->offset_width;
}

int32_t ts::HFBand::firstOffset(uint32_t channel) const
{
    ChannelsRangeList::const_iterator it(getRange(channel));
    return it == _channels.end() ? 0 : it->first_offset;
}

int32_t ts::HFBand::lastOffset(uint32_t channel) const
{
    ChannelsRangeList::const_iterator it(getRange(channel));
    return it == _channels.end() ? 0 : it->last_offset;
}


//----------------------------------------------------------------------------
// Create an HFBand from an XML element. Null pointer on error.
//----------------------------------------------------------------------------

ts::HFBandPtr ts::HFBand::FromXML(const xml::Element* elem)
{
    // Get the content of the <hfband> element.
    BandType type = UHF;
    xml::ElementVector regions;
    xml::ElementVector channels;
    bool success =
        elem->getIntEnumAttribute(type, BandTypeEnum, u"type", true) &&
        elem->getChildren(regions, u"region", 1) &&
        elem->getChildren(channels, u"channels", 1);

    if (!success) {
        elem->report().error(u"Error in <%s> at line %d", {elem->name(), elem->lineNumber()});
        return HFBandPtr();
    }

    // Build a new HFBand object.
    HFBandPtr hf(new HFBand(type));

    // Build list of regions.
    for (size_t i = 0; i < regions.size(); ++i) {
        UString name;
        if (regions[i]->getAttribute(name, u"name", true)) {
            hf->_regions.push_back(name);
        }
        else {
            success = false;
        }
    }

    // Build ranges of channels.
    for (size_t i = 0; i < channels.size(); ++i) {
        ChannelsRange chan;
        const bool ok = 
            channels[i]->getIntAttribute<uint32_t>(chan.first_channel, u"first_channel", true) &&
            channels[i]->getIntAttribute<uint32_t>(chan.last_channel, u"last_channel", true, 0, chan.first_channel) &&
            channels[i]->getIntAttribute<uint64_t>(chan.base_frequency, u"base_frequency", true) &&
            channels[i]->getIntAttribute<uint64_t>(chan.channel_width, u"channel_width", true) &&
            channels[i]->getIntAttribute<int32_t>(chan.first_offset, u"first_offset", false, 0) &&
            channels[i]->getIntAttribute<int32_t>(chan.last_offset, u"last_offset", false, 0, chan.first_offset) &&
            channels[i]->getIntAttribute<uint64_t>(chan.offset_width, u"offset_width", false, 0);
        success = success && ok;
        if (ok) {
            // Insert the channels range in the list.
            // Make "next" point after expected position of new channels range.
            auto next = hf->_channels.begin();
            while (next != hf->_channels.end() && next->last_channel < chan.first_channel) {
                ++next;
            }
            if (next != hf->_channels.end() && next->first_channel <= chan.last_channel) {
                elem->report().error(u"overlapping channel numbers, line %s", {channels[i]->lineNumber()});
                success = false;
            }
            else {
                hf->_channels.insert(next, chan);
                hf->_channel_count += chan.last_channel + 1 - chan.first_channel;
            }
        }
    }

    return success ? hf : HFBandPtr();
}


//----------------------------------------------------------------------------
// An index in the repository of HFBand.
//----------------------------------------------------------------------------

ts::HFBand::HFBandIndex::HFBandIndex(BandType typ, const UString& reg) :
    type(typ),
    region(reg.toLower().toRemoved(SPACE))
{
}

ts::UString ts::HFBand::HFBandIndex::toString() const
{
    return UString::Format(u"%s for %s", {BandTypeEnum.name(type), region});
}

bool ts::HFBand::HFBandIndex::operator==(const HFBandIndex& other) const
{
    return type == other.type && region == other.region;
}

bool ts::HFBand::HFBandIndex::operator<(const HFBandIndex& other) const
{
    return type < other.type || (type == other.type && region < other.region);
}


//----------------------------------------------------------------------------
// Repository of known bands.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::HFBand::HFBandRepository);

ts::HFBand::HFBandRepository::HFBandRepository() :
    _mutex(),
    _default_region(),
    _objects()
{
}


//----------------------------------------------------------------------------
// Load the repository if not already done. Return false on error.
//----------------------------------------------------------------------------

bool ts::HFBand::HFBandRepository::load(Report& report)
{
    // Lock access to the repository.
    Guard lock(_mutex);

    // If already loaded, fine.
    if (!_objects.empty()) {
        return true;
    }

    // Get the default region.
    _default_region = DuckConfigFile::Instance()->value(u"default.region", u"europe");

    // Load the repository XML file. Search it in TSDuck directory.
    xml::Document doc(report);
    if (!doc.load(u"tsduck.hfbands.xml", true)) {
        return false;
    }

    // Load the XML model. Search it in TSDuck directory.
    xml::Document model(report);
    if (!model.load(u"tsduck.hfbands.model.xml", true)) {
        report.error(u"Model for TSDuck HF Band XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!doc.validate(model)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();

    // Analyze all <hfband> in the document.
    bool success = true;
    for (const xml::Element* node = root == nullptr ? nullptr : root->firstChildElement(); node != nullptr; node = node->nextSiblingElement()) {
        // Since the document was validated, we assume that all elements in root are <hfband>.
        HFBandPtr hf(FromXML(node));
        if (hf.isNull()) {
            success = false;
        }
        else {
            // Add the object in the repository.
            for (auto it = hf->_regions.begin(); it != hf->_regions.end(); ++it) {
                const HFBandIndex index(hf->_type, *it);
                if (_objects.find(index) != _objects.end()) {
                    report.error(u"duplicate definition for %s, line %d", {index, node->lineNumber()});
                    success = false;
                }
                else {
                    _objects[index] = hf;
                }
            }
        }
    }
    return success;
}


//----------------------------------------------------------------------------
// Get an object from the repository.
//----------------------------------------------------------------------------

ts::HFBandPtr ts::HFBand::HFBandRepository::get(BandType type, const UString& region, Report& report) const
{
    // Lock access to the repository.
    Guard lock(_mutex);

    const HFBandIndex index(type, region.empty() ? _default_region : region);
    const auto it = _objects.find(index);
    if (it != _objects.end()) {
        return it->second;
    }
    else {
        report.warning(u"no definition for %s", {index});
        return HFBandPtr(new HFBand);
    }
}
