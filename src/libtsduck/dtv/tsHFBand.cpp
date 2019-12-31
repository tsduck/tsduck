//----------------------------------------------------------------------------
//
//  TSDuck - The MPEG Transport Stream Toolkit
//  Copyright (c) 2005-2020, Thierry Lelegard
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
// Get the type of HF band as a string.
//----------------------------------------------------------------------------

ts::UString ts::HFBand::typeName() const
{
    return HFBandRepository::Instance()->bandTypeEnum.name(_type);
}


//----------------------------------------------------------------------------
// GetBand static method.
//----------------------------------------------------------------------------

const ts::HFBand* ts::HFBand::GetBand(const UString& region, BandType type, Report& report)
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

void ts::HFBand::SetDefaultRegion(const ts::UString& region, ts::Report& report)
{
    HFBandRepository* repo = HFBandRepository::Instance();
    repo->load(report);
    repo->setDefaultRegion(region);
}

ts::UStringList ts::HFBand::GetAllRegions(Report& report)
{
    HFBandRepository* repo = HFBandRepository::Instance();
    repo->load(report);
    return repo->allRegions();
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
    return it == _channels.end() ? 0 : it->frequency(channel, offset);
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
// Lowest and highest frequency in band.
//----------------------------------------------------------------------------

uint64_t ts::HFBand::lowestFrequency(bool strict) const
{
    return _channels.empty() ? 0 : _channels.front().lowestFrequency(strict);
}

uint64_t ts::HFBand::highestFrequency(bool strict) const
{
    return _channels.empty() ? 0 : _channels.back().highestFrequency(strict);
}


//----------------------------------------------------------------------------
// Lowest and highest frequency in range.
//----------------------------------------------------------------------------

uint64_t ts::HFBand::ChannelsRange::lowestFrequency(bool strict) const
{
    if (strict) {
        return base_frequency + int64_t(first_offset) * int64_t(offset_width);
    }
    else {
        return base_frequency - channel_width / 2;
    }
}

uint64_t ts::HFBand::ChannelsRange::highestFrequency(bool strict) const
{
    if (strict) {
        return base_frequency + (last_channel - first_channel) * channel_width + int64_t(last_offset) * int64_t(offset_width);
    }
    else {
        return base_frequency + (last_channel - first_channel) * channel_width + channel_width / 2;
    }
}

uint64_t ts::HFBand::ChannelsRange::frequency(uint32_t channel, int32_t offset) const
{
    return base_frequency + (channel - first_channel) * channel_width + int64_t(offset * int64_t(offset_width));
}

uint32_t ts::HFBand::ChannelsRange::channelNumber(uint64_t frequency) const
{
    return channel_width == 0 ? 0 : first_channel + uint32_t((frequency - base_frequency + channel_width / 2) / channel_width);
}


//----------------------------------------------------------------------------
// Compute a channel or offset number from a frequency.
//----------------------------------------------------------------------------

bool ts::HFBand::inBand(uint64_t frequency, bool strict) const
{
    // Loop on all ranges of channels.
    for (auto it = _channels.begin(); it != _channels.end(); ++it) {
        // If the frequency is in this range.
        if (frequency >= it->lowestFrequency(strict) && frequency <= it->highestFrequency(strict)) {
            if (strict) {
                // Check all channels individually.
                uint64_t freq = it->base_frequency;
                for (int32_t off = it->first_offset; off <= it->last_offset; ++off) {
                    if (frequency >= freq + int64_t(it->first_offset) * int64_t(it->offset_width) &&
                        frequency <= freq + int64_t(it->last_offset) * int64_t(it->offset_width))
                    {
                        return true;
                    }
                    freq += it->channel_width;
                }
                return false; // not in a strict channel
            }
            else {
                // We are inside the band.
                return true;
            }
        }
    }
    return false; // not found
}

uint32_t ts::HFBand::channelNumber(uint64_t frequency) const
{
    for (auto it = _channels.begin(); it != _channels.end(); ++it) {
        if (frequency >= it->lowestFrequency(true) && frequency <= it->highestFrequency(true)) {
            return it->channelNumber(frequency);
        }
    }
    return 0; // not found
}

int32_t ts::HFBand::offsetCount(uint64_t frequency) const
{
    for (auto it = _channels.begin(); it != _channels.end(); ++it) {
        if (it->offset_width > 0 && frequency >= it->lowestFrequency(true) && frequency <= it->highestFrequency(true)) {
            const int32_t off = int32_t(int64_t(frequency) - int64_t(it->frequency(it->channelNumber(frequency), 0)));
            const int32_t count = (std::abs(off) + int32_t(it->offset_width / 2)) / int32_t(it->offset_width);
            return off < 0 ? -count : count;
        }
    }
    return 0; // not found
}


//----------------------------------------------------------------------------
// Return a human-readable description of a channel.
//----------------------------------------------------------------------------

ts::UString ts::HFBand::description(uint32_t channel, int32_t offset, int strength, int quality) const
{
    const uint64_t freq = frequency(channel, offset);
    const int mhz = int(freq / 1000000);
    const int khz = int((freq % 1000000) / 1000);
    UString desc(HFBandRepository::Instance()->bandTypeEnum.name(_type));
    desc += u" channel ";
    desc += UString::Decimal(channel);
    if (offset != 0) {
        desc += u", offset ";
        desc += UString::Decimal(offset, 0, true, u",", true);
    }
    desc += u" (";
    desc += UString::Decimal(mhz);
    if (khz > 0) {
        desc += UString::Format(u".%03d", {khz});
    }
    desc += u" MHz)";
    if (strength >= 0) {
        desc += UString::Format(u", strength: %d%%", {strength});
    }
    if (quality >= 0) {
        desc += UString::Format(u", quality: %d%%", {quality});
    }
    return desc;
}


//----------------------------------------------------------------------------
// Create an HFBand from an XML element. Null pointer on error.
//----------------------------------------------------------------------------

ts::HFBand::HFBandPtr ts::HFBand::FromXML(const xml::Element* elem)
{
    // Get the content of the <hfband> element.
    BandType type = UHF;
    xml::ElementVector regions;
    xml::ElementVector channels;
    bool success =
        elem->getIntEnumAttribute(type, HFBandRepository::Instance()->bandTypeEnum, u"type", true) &&
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
    return UString::Format(u"%s band in region %s", {HFBandRepository::Instance()->bandTypeEnum.name(type), region});
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
    bandTypeEnum({{u"VHF", ts::HFBand::VHF}, {u"UHF", ts::HFBand::UHF}}),
    _mutex(),
    _default_region(),
    _objects(),
    _allRegions(),
    _voidBand(new HFBand)
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

    // Get the default region from configuration file.
    setDefaultRegion(UString());

    // A set of region names (to build a list of unique names).
    std::set<UString> regionSet;

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
                // Get band type + region name.
                const HFBandIndex index(hf->_type, *it);
                // Build a set of unique entries for region names.
                regionSet.insert(*it);
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

    // Build a sorted list of region names.
    for (auto it = regionSet.begin(); it != regionSet.end(); ++it) {
        _allRegions.push_back(*it);
    }

    return success;
}


//----------------------------------------------------------------------------
// Get/set the default region.
//----------------------------------------------------------------------------

ts::UString ts::HFBand::HFBandRepository::defaultRegion() const
{
    Guard lock(_mutex);
    return _default_region;
}

void ts::HFBand::HFBandRepository::setDefaultRegion(const UString& region)
{
    Guard lock(_mutex);
    // If the region is empty, get the one for configuration file.
    _default_region = region.empty() ? DuckConfigFile::Instance()->value(u"default.region", u"europe") : region;
}


//----------------------------------------------------------------------------
// Get an object from the repository.
//----------------------------------------------------------------------------

const ts::HFBand* ts::HFBand::HFBandRepository::get(BandType type, const UString& region, Report& report) const
{
    // Lock access to the repository.
    Guard lock(_mutex);

    const HFBandIndex index(type, region.empty() ? _default_region : region);
    const auto it = _objects.find(index);
    if (it != _objects.end()) {
        return it->second.pointer();
    }
    else {
        report.warning(u"no definition for %s", {index});
        return _voidBand.pointer();
    }
}
