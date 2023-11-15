//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsLNB.h"
#include "tsAlgorithm.h"
#include "tsxmlModelDocument.h"
#include "tsxmlElement.h"
#include "tsDuckConfigFile.h"


//----------------------------------------------------------------------------
// Constructors and destructors.
//----------------------------------------------------------------------------

ts::LNB::LNB(const UString& name, Report& report)
{
    set(name, report);
}

ts::LNB::LNB(uint64_t frequency)
{
    set(frequency);
}

ts::LNB::LNB(uint64_t low_frequency, uint64_t high_frequency, uint64_t switch_frequency)
{
    set(low_frequency, high_frequency, switch_frequency);
}


//----------------------------------------------------------------------------
// Check if the LNB is polarization-controlled.
//----------------------------------------------------------------------------

bool ts::LNB::isPolarizationControlled() const
{
    return !_bands.empty() && _bands[0].polarity != POL_NONE && _bands[0].polarity != POL_AUTO;
}


//----------------------------------------------------------------------------
// Get the legacy frequency values.
//----------------------------------------------------------------------------

uint64_t ts::LNB::legacyLowOscillatorFrequency() const
{
    return ((_bands.size() == 1 || _bands.size() == 2) && !isPolarizationControlled()) ? _bands[0].oscillator : 0;
}

uint64_t ts::LNB::legacyHighOscillatorFrequency() const
{
    return (_bands.size() == 2 && !isPolarizationControlled()) ? _bands[1].oscillator : 0;
}

uint64_t ts::LNB::legacySwitchFrequency() const
{
    return (_bands.size() == 2 && !isPolarizationControlled()) ? _bands[0].switch_freq : 0;
}


//----------------------------------------------------------------------------
// Compute the intermediate frequency from a satellite carrier frequency.
//----------------------------------------------------------------------------

bool ts::LNB::transpose(Transposition& transposition, uint64_t sat_freq, Polarization polarity, Report& report) const
{
    // Reset transposition data.
    transposition.satellite_frequency = sat_freq;
    transposition.intermediate_frequency = 0;
    transposition.oscillator_frequency = 0;
    transposition.stacked = false;
    transposition.band_index = 0;

    // We need to know the polarity on polarization-controlled LNB.
    const bool pol_control = isPolarizationControlled();
    if (pol_control && (polarity == POL_NONE || polarity == POL_AUTO)) {
        report.error(u"no polarization provided on polarization-controlled LNB, type %s", {_name});
        return false;
    }

    // Look for the right band for the target frequency.
    size_t index = 0;
    if (pol_control) {
        // Need to match the frequency band and polarization at the same time.
        while (index < _bands.size() && (sat_freq < _bands[index].low || sat_freq > _bands[index].high || polarity != _bands[index].polarity)) {
            ++index;
        }
    }
    else {
        // Need to match the frequency band.
        while (index < _bands.size() && (sat_freq < _bands[index].low || sat_freq > _bands[index].high)) {
            ++index;
        }
        // If we found a band but the sat freq is above the switch frequency => use next band, if there is one.
        if (index + 1 < _bands.size() && _bands[index].switch_freq != 0 && sat_freq > _bands[index].switch_freq) {
            ++index;
        }
    }

    // Now compute the transposition.
    if (index >= _bands.size()) {
        report.error(u"satellite frequency %'d Hz cannot be transposed using LNB type %s", {sat_freq, _name});
        return false;
    }
    else {
        transposition.oscillator_frequency = _bands[index].oscillator;
        transposition.intermediate_frequency = sat_freq < _bands[index].oscillator ? _bands[index].oscillator - sat_freq : sat_freq - _bands[index].oscillator;
        transposition.stacked = pol_control;
        transposition.band_index = index;
        return true;
    }
}


//----------------------------------------------------------------------------
// Convert to a string object
//----------------------------------------------------------------------------

ts::UString ts::LNB::toString() const
{
    // If a command line safe alias is available, use it.
    return _alias.empty() ? _name : _alias;
}


//----------------------------------------------------------------------------
// Interpret a string as an LNB value.
//----------------------------------------------------------------------------

bool ts::LNB::set(const UString& name, Report& report)
{
    // Try to find a matching name of alias in the repository.
    const LNB* lnb = LNBRepository::Instance().get(name, report);
    if (lnb != nullptr) {
        // Found a matching name in the repository.
        *this = *lnb;
        return true;
    }

    // Try to interpret the string as legacy format.
    uint64_t low_freq = 0;
    uint64_t high_freq = 0;
    uint64_t switch_freq = 0;
    if (name.toInteger(low_freq)) {
        // Legacy "freq" format. Frequencies in MHz.
        set(low_freq * 1000000);
        return true;
    }
    else if (name.scan(u"%d,%d,%d", {&low_freq, &high_freq, &switch_freq})) {
        // "low,high,switch" legacy format. Frequencies in MHz.
        set(low_freq * 1000000, high_freq * 1000000, switch_freq * 1000000);
        return true;
    }
    else {
        report.error(u"unknown LNB name \"%s\"", {name});
        return false;
    }
}


//----------------------------------------------------------------------------
// Set values of a legacy LNB with low and high band.
//----------------------------------------------------------------------------

void ts::LNB::set(uint64_t frequency)
{
    set(frequency, 0, 0);
}

void ts::LNB::set(uint64_t low_frequency, uint64_t high_frequency, uint64_t switch_frequency)
{
    if (high_frequency == 0 && switch_frequency == 0) {
        // No high band, just an oscillator frequency and "unlimited" band.
        _name.format(u"%d", {low_frequency / 1000000});
        _alias.clear();
        _bands.clear();
        _bands.resize(1);
        _bands[0].low = 0;
        _bands[0].high = std::numeric_limits<uint64_t>::max();
        _bands[0].oscillator = low_frequency;
    }
    else {
        // High band, two oscillators (low and high). The switch frequency is the boundary between the two bands.
        _name.format(u"%d,%d,%d", {low_frequency / 1000000, high_frequency / 1000000, switch_frequency / 1000000});
        _alias.clear();
        _bands.clear();
        _bands.resize(2);
        _bands[0].low = 0;
        _bands[0].high = switch_frequency;
        _bands[0].oscillator = low_frequency;
        _bands[0].switch_freq = switch_frequency;
        _bands[1].low = switch_frequency;
        _bands[1].high = std::numeric_limits<uint64_t>::max();
        _bands[1].oscillator = high_frequency;
    }
}


//----------------------------------------------------------------------------
// Get a list of all available LNB's from the configuration file.
//----------------------------------------------------------------------------

ts::UStringList ts::LNB::GetAllNames(Report& report)
{
    return LNBRepository::Instance().allNames(report);
}


//----------------------------------------------------------------------------
// Repository of known LNB's.
//----------------------------------------------------------------------------

TS_DEFINE_SINGLETON(ts::LNB::LNBRepository);

ts::LNB::LNBRepository::LNBRepository()
{
}


//----------------------------------------------------------------------------
// List of available LNB names.
//----------------------------------------------------------------------------

const ts::UStringList& ts::LNB::LNBRepository::allNames(Report& report)
{
    // Lock access to the repository. Load XML file if not already done.
    std::lock_guard<std::mutex> lock(_mutex);
    load(report);

    // After loading, the _names list won't change, return a constant reference to it.
    return _names;
}


//----------------------------------------------------------------------------
// Convert a name to an index in LNB map.
//----------------------------------------------------------------------------

ts::UString ts::LNB::LNBRepository::ToIndex(const UString& name)
{
    UString n(name);
    n.convertToLower();
    n.remove(SPACE);
    return n;
}


//----------------------------------------------------------------------------
// Get an LNB by name or alias from the repository.
//----------------------------------------------------------------------------

const ts::LNB* ts::LNB::LNBRepository::get(const UString& name, Report& report)
{
    // Lock access to the repository.
    std::lock_guard<std::mutex> lock(_mutex);

    if (!load(report)) {
        // Error loading XML configuration file.
        return nullptr;
    }
    else if (name.empty()) {
        // Get default LNB. Null pointer if repository is empty.
        return _default_lnb.pointer();
    }
    else {
        // Lookup by name, lower case, without space.
        const auto it(_lnbs.find(ToIndex(name)));
        return it == _lnbs.end() ? nullptr : it->second.pointer();
    }
}


//----------------------------------------------------------------------------
// Get name attribute of an <lnb> or <alias> element.
//----------------------------------------------------------------------------

bool ts::LNB::LNBRepository::getNameAttribute(const xml::Element* node, UString& name, UStringList& index_names)
{
    // The attribute must be present and not empty.
    if (!node->getAttribute(name, u"name", true, UString(), 1)) {
        return false;
    }

    // Check if the name is already known.
    const UString iname(ToIndex(name));
    if (Contains(_lnbs, iname)) {
        node->report().error(u"duplicate LNB name '%s' in <%s> line %d", {name, node->name(), node->lineNumber()});
        return false;
    }

    // Store full name in global list of LNB names and aliases.
    _names.push_back(name);

    // Store normalized name in local list of indexes for this LNB.
    index_names.push_back(iname);
    return true;
}


//----------------------------------------------------------------------------
// Load the repository if not already done. Return false on error.
//----------------------------------------------------------------------------

bool ts::LNB::LNBRepository::load(Report& report)
{
    // If already loaded, fine.
    if (!_lnbs.empty()) {
        return true;
    }

    // Load the repository XML file. Search it in TSDuck directory.
    xml::Document doc(report);
    if (!doc.load(u"tsduck.lnbs.xml", true)) {
        return false;
    }

    // Load the XML model. Search it in TSDuck directory.
    xml::ModelDocument model(report);
    if (!model.load(u"tsduck.lnbs.model.xml", true)) {
        report.error(u"Model for TSDuck LNB XML files not found");
        return false;
    }

    // Validate the input document according to the model.
    if (!model.validate(doc)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();

    // Analyze all <lnb> in the document.
    bool success = true;
    for (const xml::Element* node = root == nullptr ? nullptr : root->firstChildElement(); node != nullptr; node = node->nextSiblingElement()) {

        // Allocate LNB object.
        LNBPtr lnb(new LNB);
        if (lnb.isNull()) {
            success = false;
            break;
        }

        // Since the document was validated, we assume that all elements in root are <lnb>.
        UStringList index_names;
        bool is_default = false;
        xml::ElementVector xalias;
        xml::ElementVector xband;

        // Get <lnb> element.
        bool lnb_ok =
            getNameAttribute(node, lnb->_name, index_names) &&
            node->getBoolAttribute(is_default, u"default", false, false) &&
            node->getChildren(xalias, u"alias") &&
            node->getChildren(xband, u"band", 1);

        // Get all aliases. Don't stop on error.
        for (auto it : xalias) {
            UString alias;
            lnb_ok = getNameAttribute(it, alias, index_names) && lnb_ok;
            // Check if the alias is suitable for command line usage.
            if (lnb_ok && lnb->_alias.empty()) {
                bool ok = true;
                for (size_t i = 0; ok && i < alias.size(); ++i) {
                    const UChar c = alias[i];
                    ok = IsAlpha(c) || IsDigit(c) || c == u'-' || c == u'_' || c == u',' || c == u':';
                }
                if (ok) {
                    lnb->_alias = alias;
                }
            }
        }

        // Get all bands.
        for (auto it : xband) {
            Band band;
            const bool band_ok =
                it->getIntAttribute<uint64_t>(band.low, u"low", true) &&
                it->getIntAttribute<uint64_t>(band.high, u"high", true) &&
                it->getIntAttribute<uint64_t>(band.oscillator, u"oscillator", true) &&
                it->getIntAttribute<uint64_t>(band.switch_freq, u"switch", false, 0) &&
                it->getIntEnumAttribute<Polarization>(band.polarity, PolarizationEnum, u"polarity", false, POL_NONE);
            if (band_ok) {
                lnb->_bands.push_back(band);
            }
            else {
                lnb_ok = false;
            }
        }

        // Register the new LNB with its name and all aliases.
        if (lnb_ok) {
            for (const auto& it : index_names) {
                _lnbs.insert(std::make_pair(it, lnb));
            }
            // The last <lnb> with default="true" is the default one.
            // If there is no explicit default, the first <lnb> is the default one.
            if (_default_lnb.isNull() || is_default) {
                _default_lnb = lnb;
            }
        }

        success = success && lnb_ok;
    }

    // Override the default LNB if specified in the TSDuck configuration file.
    const UString def_name(DuckConfigFile::Instance().value(u"default.lnb"));
    const UString def_index(ToIndex(def_name));
    if (!def_index.empty()) {
        auto it = _lnbs.find(def_index);
        if (it != _lnbs.end()) {
            _default_lnb = it->second;
        }
        else {
            report.error(u"default LNB \"%s\" not found", {def_name});
        }
    }

    // Build a sorted list of LNB names and aliases.
    _names.sort();
    return success;
}
