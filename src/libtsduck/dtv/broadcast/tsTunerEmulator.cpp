//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2021, Thierry Lelegard
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
//-----------------------------------------------------------------------------

#include "tsTunerEmulator.h"
#include "tsDuckContext.h"
#include "tsxmlElement.h"
#include "tsxmlModelDocument.h"
TSDUCK_SOURCE;


//-----------------------------------------------------------------------------
// Constructors and destructors.
//-----------------------------------------------------------------------------

ts::TunerEmulator::TunerEmulator(DuckContext& duck) :
    TunerBase(duck),
    _delivery_systems(),
    _xml_file_path(),
    _info_only(false),
    _state(State::CLOSED),
    _file(),
    _channels(),
    _tune_index(0),
    _tune_frequency(0),
    _strength(-1)
{
}

ts::TunerEmulator::~TunerEmulator()
{
}


//-----------------------------------------------------------------------------
// Description of a channel.
//-----------------------------------------------------------------------------

ts::TunerEmulator::Channel::Channel() :
    frequency(0),
    bandwidth(0),
    delivery(DS_UNDEFINED),
    file()
{
}

// Compute the distance of a frequency from the center one.
uint64_t ts::TunerEmulator::Channel::distance(uint64_t freq) const
{
    return uint64_t(std::abs(int64_t(frequency) - int64_t(freq)));
}

// Check if a frequency is in the channel.
bool ts::TunerEmulator::Channel::inBand(uint64_t freq) const
{
    return distance(freq) < std::max<uint64_t>(1, bandwidth / 2);
}

// Compute the virtual signal strength for a given frequency.
int ts::TunerEmulator::Channel::strength(uint64_t freq) const
{
    const uint64_t dist = distance(freq);
    const uint64_t max = std::max<uint64_t>(1, bandwidth / 2);
    // Emulate a strength: 100% at center frequency, 50% at end of bandwidth.
    return (dist > max) ? 0 : int(50 + (50 * (max - dist)) / max);
}


//-----------------------------------------------------------------------------
// Open the tuner emulator.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::open(const UString& device_name, bool info_only, Report& report)
{
    // Check state.
    if (_state != State::CLOSED) {
        report.error(u"internal error, tuner emulator is not in closed state");
        return false;
    }

    // Get absolute path of XML file directory (in case of relative paths in the file).
    const UString base_directory(DirectoryName(AbsoluteFilePath(device_name)));

    // Reset channel descriptions.
    _delivery_systems.clear();
    _channels.clear();

    // Open and validate the XML file describing the tuner emulator.
    xml::Document doc(report);
    xml::ModelDocument model(report);
    report.debug(u"load tuner emulator from %s", {device_name});
    if (!doc.load(device_name, false) || !model.load(u"tsduck.etuner.model.xml", true) || !model.validate(doc)) {
        return false;
    }

    // Get the root in the document. Should be ok since we validated the document.
    const xml::Element* root = doc.rootElement();
    assert(root != nullptr);

    // Get default values;
    DeliverySystem def_delivery = DS_UNDEFINED;
    uint64_t def_bandwidth = 0;
    UString def_directory;
    const xml::Element* def = root->findFirstChild(u"defaults", true);
    bool success = true;
    if (def != nullptr) {
        success = def->getIntEnumAttribute(def_delivery, DeliverySystemEnum, u"delivery", false, DS_UNDEFINED) &&
                  def->getIntAttribute(def_bandwidth, u"bandwidth", false, 0) &&
                  def->getAttribute(def_directory, u"directory", false);
        if (def_directory.empty()) {
            def_directory = base_directory;
        }
        else {
            def_directory = AbsoluteFilePath(def_directory, base_directory);
        }
        report.debug(u"defaults: delivery: %s, bandwidth: %'d Hz, directory: %s", {DeliverySystemEnum.name(def_delivery), def_bandwidth, def_directory});
    }

    // Get all channel descriptions.
    xml::ElementVector xchannels;
    success = success && root->getChildren(xchannels, u"channel");
    _channels.reserve(xchannels.size());
    for (auto it = xchannels.begin(); success && it != xchannels.end(); ++it) {
        Channel chan;
        success = (*it)->getIntAttribute(chan.frequency, u"frequency", true) &&
                  (*it)->getIntAttribute(chan.bandwidth, u"bandwidth", false, def_bandwidth) &&
                  (*it)->getIntEnumAttribute(chan.delivery, DeliverySystemEnum, u"delivery", false, def_delivery) &&
                  (*it)->getAttribute(chan.file, u"file", true);
        if (success) {
            chan.file = AbsoluteFilePath(chan.file, def_directory);
        }
        _delivery_systems.insert(chan.delivery);
        _channels.push_back(chan);
    }
    report.debug(u"loaded %d emulated channels", {_channels.size()});

    if (success) {
        _xml_file_path = device_name;
        _info_only = info_only;
        _state = State::OPEN;
        return true;
    }
    else {
        report.error(u"error opening tuner emulator at %s", {device_name});
        _delivery_systems.clear();
        _channels.clear();
        return false;
    }
}


//-----------------------------------------------------------------------------
// Close the tuner emulator.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::close(Report& report)
{
    // Stop reception (close resources).
    stop(report);

    // Cleanup internal state.
    _channels.clear();
    _delivery_systems.clear();
    _xml_file_path.clear();
    _info_only = false;
    _tune_index = 0;
    _tune_frequency = 0;
    _strength = -1;
    _state = State::CLOSED;
    return true;
}


//-----------------------------------------------------------------------------
// Basic information.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::isOpen() const
{
    return _state != State::CLOSED;
}

bool ts::TunerEmulator::infoOnly() const
{
    return _info_only;
}

const ts::DeliverySystemSet& ts::TunerEmulator::deliverySystems() const
{
    return _delivery_systems;
}

ts::UString ts::TunerEmulator::deviceName() const
{
    return _xml_file_path;
}

ts::UString ts::TunerEmulator::deviceInfo() const
{
    return _xml_file_path;
}

ts::UString ts::TunerEmulator::devicePath() const
{
    return _xml_file_path;
}


//-----------------------------------------------------------------------------
// Emulated signal characteristics.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::signalLocked(Report& report)
{
    return _state == State::TUNED || _state == State::STARTED;
}

int ts::TunerEmulator::signalStrength(Report& report)
{
    return _strength;
}

int ts::TunerEmulator::signalQuality(Report& report)
{
    // Use same percentage as signal strength.
    return _strength;
}


//-----------------------------------------------------------------------------
// Tune to a frequency
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::tune(ModulationArgs& params, Report& report)
{
    // Cannot tune if closed or started.
    if (_state == State::CLOSED || _state == State::STARTED) {
        report.error(u"cannot tune, wrong tuner emulator state");
        return false;
    }

    // We only look as those parameters:
    const uint64_t freq = params.frequency.value(0);
    const DeliverySystem delsys = params.delivery_system.value(DS_UNDEFINED);
    if (freq == 0) {
        report.error(u"frequency unspecified");
        return false;
    }

    // Look for the first channel into which the frequency falls.
    size_t index = 0;
    while (index < _channels.size() && !_channels[index].inBand(freq)) {
        index++;
    }
    if (index >= _channels.size()) {
        report.error(u"no signal at %'d Hz", {freq});
        return false;
    }
    else if (delsys != DS_UNDEFINED && _channels[index].delivery != DS_UNDEFINED && delsys != _channels[index].delivery) {
        report.error(u"delivery system at %'d Hz is %s, %s requested ", {freq, DeliverySystemEnum.name(_channels[index].delivery), DeliverySystemEnum.name(delsys)});
        return false;
    }

    // Tuned !
    _tune_index = index;
    _tune_frequency = freq;
    _strength = _channels[index].strength(freq);
    _state = State::TUNED;
    return true;
}


//-----------------------------------------------------------------------------
// Start / stop reception.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::start(Report& report)
{
    if (_state != State::TUNED) {
        report.error(u"cannot start reception, wrong tuner emulator state");
        return false;
    }

    assert(!_file.isOpen());
    assert(_tune_index < _channels.size());

    const Channel& chan(_channels[_tune_index]);
    if (chan.file.empty()) {
        report.error(u"empty file name for channel at %'d Hz", {chan.frequency});
        return false;
    }
    else if (!_file.openRead(chan.file, 0, 0, report)) {
        return false;
    }

    // Started !
    _state = State::STARTED;
    return true;
}

bool ts::TunerEmulator::stop(Report& report)
{
    // Close resources, regardless of state.
    if (_file.isOpen()) {
        _file.close(report);
    }
    // Change state only if started.
    if (_state == State::STARTED) {
        _state = State::TUNED;
    }
    return true;
}


//-----------------------------------------------------------------------------
// Packet reception.
//-----------------------------------------------------------------------------

size_t ts::TunerEmulator::receive(TSPacket* buffer, size_t max_packets, const AbortInterface* abort, Report& report)
{
    if (_state == State::STARTED && _file.isOpen()) {
        return _file.readPackets(buffer, nullptr, max_packets, report);
    }
    else {
        return 0;  // error
    }
}


//-----------------------------------------------------------------------------
// Get the current "tuning" parameters.
//-----------------------------------------------------------------------------

bool ts::TunerEmulator::getCurrentTuning(ModulationArgs& params, bool reset_unknown, Report& report)
{
    if (reset_unknown) {
        params.reset();
    }
    if (_state == State::CLOSED || _state == State::OPEN) {
        return false; // not tuned
    }
    else {
        assert(_tune_index < _channels.size());
        params.frequency = _tune_frequency;
        params.delivery_system = _channels[_tune_index].delivery;
        return true;
    }
}


//-----------------------------------------------------------------------------
// Display the current tuner emulator state.
//-----------------------------------------------------------------------------

std::ostream& ts::TunerEmulator::displayStatus(std::ostream& strm, const UString& margin, Report& report, bool extended)
{
    if (_state == State::TUNED || _state == State::STARTED) {
        assert(_tune_index < _channels.size());
        strm << "Current:" << std::endl;
        strm << "  Delivery system: " << DeliverySystemEnum.name(_channels[_tune_index].delivery) << std::endl;
        strm << "  Frequency: " << UString::Decimal(_tune_frequency) << " Hz" << std::endl;
        strm << "  Signal strength: " << _strength << " %" << std::endl;
        strm << std::endl;
    }
    strm << "Number of active channels: " << _channels.size() << std::endl;
    for (size_t i = 0; i < _channels.size(); ++i) {
        const Channel& chan(_channels[i]);
        strm << "  " << UString::Decimal(chan.frequency) << " Hz (" << DeliverySystemEnum.name(chan.delivery)
             << ", width: " << UString::Decimal(chan.bandwidth) << ")";
        if (!chan.file.empty()) {
            strm << " file: " << chan.file;
        }
        strm << std::endl;
    }
    return strm;
}
