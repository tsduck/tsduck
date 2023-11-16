//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/#license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Definition of an HF frequency band (UHF, VHF).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsModulation.h"
#include "tsSafePtr.h"
#include "tsCerrReport.h"
#include "tsSingleton.h"
#include "tsxml.h"

namespace ts {
    //!
    //! Definition of an HF frequency band (UHF, VHF).
    //! @ingroup hardware
    //!
    //! Each region or country has it own definitions of the frequencies bands and layouts.
    //! The most common types of frequency bands are VHF (Very High Frequency) and UHF
    //! (Ultra High Frequency) for terrestrial TV. Some countries also define predefined
    //! layouts for satellite frequency bands. This is the case for Japan with the "BS"
    //! and "CS" satellite bands.
    //!
    //! There is a repository of known HF bands layout per country or region.
    //! This repository is read from an XML file. There is only one instance of HFBand
    //! per country or region.
    //! @see ts::HFBand::GetBand()
    //!
    class TSDUCKDLL HFBand
    {
        TS_NOBUILD_NOCOPY(HFBand);
    public:
        //!
        //! Get the default region.
        //! @param [in,out] report Where to report errors.
        //! @return The default region. This is the value of the parameter "default.region"
        //! in the TSDuck configuration file for the current application. If undefined in
        //! the configuration file, the default is "europe".
        //!
        static UString DefaultRegion(Report& report = CERR);

        //!
        //! Set the default region.
        //! @param [in] region The new region to use as default. If empty, then use the value
        //! of the parameter "default.region" in the TSDuck configuration file for the current
        //! application. If undefined in the configuration file, the default is "europe".
        //! @param [in,out] report Where to report errors.
        //!
        static void SetDefaultRegion(const UString& region = UString(), Report& report = CERR);

        //!
        //! Get a list of all available regions from the configuration file.
        //! @param [in,out] report Where to report errors.
        //! @return The list of all available regions.
        //!
        static UStringList GetAllRegions(Report& report = CERR);

        //!
        //! Get a list of all available HF bands for a given region in the configuration file.
        //! @param [in] region The region name. If empty, then use the default region.
        //! @param [in,out] report Where to report errors.
        //! @return The list of all available regions.
        //!
        static UStringList GetAllBands(const UString& region = UString(), Report& report = CERR);

        //!
        //! Get the description of an HF band from the configuration file.
        //! @param [in] region Region of country name (not case sensitive).
        //! @param [in] band HF band type (u"UHF", u"VHF", etc).
        //! @param [in,out] report Where to report errors.
        //! @param [in] silent_band If true, do not report error message if the band is not found in
        //! the file. Other errors (HF band file not found, region not found) are still reported.
        //! @return A pointer to the instance for the corresponding @a region.
        //! If the repository contains no known band for the region, return an empty object.
        //!
        static const HFBand* GetBand(const UString& region = UString(), const UString& band = u"UHF", Report& report = CERR, bool silent_band = false);

        //!
        //! Get the name of the HF band as a string.
        //! @return The name of the HF band as a string.
        //!
        UString bandName() const { return _band_name; }

        //!
        //! Check if there is no channel in the HF band.
        //! @return True if there is no channel in the HF band (typically an invalid band).
        //!
        bool empty() const { return _channels.empty(); }

        //!
        //! Get the first channel number in the HF band.
        //! @return The first channel number in the HF band.
        //!
        uint32_t firstChannel() const { return _channels.empty() ? 0 : _channels.front().first_channel; }

        //!
        //! Get the last channel number in the HF band.
        //! @return The last channel number in the HF band.
        //!
        uint32_t lastChannel() const { return _channels.empty() ? 0 : _channels.back().last_channel; }

        //!
        //! Get the number of channels in the HF band.
        //! Note that this cannot be computed from firstChannel() and lastChannel() since
        //! an HF band can have "holes", non-existent channels.
        //! @return The number of channels in the HF band.
        //!
        uint32_t channelCount() const { return _channel_count; }

        //!
        //! Get the list of channels in the HF band as a string.
        //! @return The list of channels in the HF band as a string.
        //!
        UString channelList() const;

        //!
        //! Check if a channel is valid in the HF band.
        //! @param [in] channel Channel number.
        //! @return True if @a channel is valid, false otherwise.
        //!
        bool isValidChannel(uint32_t channel) const;

        //!
        //! Get the next channel in the HF band.
        //! @param [in] channel Channel number.
        //! @return The channel number after @a channel. Can be different from @a channel + 1
        //! when there are "holes" in the HF band. Return zero on error (invalid channel).
        //!
        uint32_t nextChannel(uint32_t channel) const;

        //!
        //! Get the previous channel in the HF band.
        //! @param [in] channel Channel number.
        //! @return The channel number before @a channel. Can be different from @a channel - 1
        //! when there are "holes" in the HF band. Return zero on error (invalid channel).
        //!
        uint32_t previousChannel(uint32_t channel) const;

        //!
        //! Get the lowest frequency in the HF band.
        //! @param [in] strict If true, @a frequency must be strictly inside the allowed offset range
        //! of a channel. When false, only check that the frequency is inside the global band.
        //! @return The frequency in Hz.
        //!
        uint64_t lowestFrequency(bool strict = false) const;

        //!
        //! Get the highest frequency in the HF band.
        //! @param [in] strict If true, @a frequency must be strictly inside the allowed offset range
        //! of a channel. When false, only check that the frequency is inside the global band.
        //! @return The frequency in Hz.
        //!
        uint64_t highestFrequency(bool strict = false) const;

        //!
        //! Get the frequency of a channel in the HF band.
        //! @param [in] channel Channel number.
        //! @param [in] offset Optional offset number.
        //! @return The frequency in Hz or zero on error (invalid channel).
        //!
        uint64_t frequency(uint32_t channel, int32_t offset = 0) const;

        //!
        //! Get the bandwidth of a channel in the HF band.
        //! @param [in] channel Channel number.
        //! @return The channel width in Hz or zero on error (invalid channel).
        //!
        uint64_t bandWidth(uint32_t channel) const;

        //!
        //! Get the offset frequency width of a channel in the HF band.
        //! @param [in] channel Channel number.
        //! @return The offset frequency width in Hz.
        //!
        uint64_t offsetWidth(uint32_t channel) const;

        //!
        //! Get the first allowed offset of a channel in the HF band.
        //! @param [in] channel Channel number.
        //! @return The first allowed offset for @a channel.
        //!
        int32_t firstOffset(uint32_t channel) const;

        //!
        //! Get the last allowed offset of a channel in the HF band.
        //! @param [in] channel Channel number.
        //! @return The last allowed offset for @a channel.
        //!
        int32_t lastOffset(uint32_t channel) const;

        //!
        //! Get the polarization of a channel in the HF band.
        //! @param [in] channel Channel number.
        //! @return The polarization for @a channel.
        //! If POL_NONE, then no polarization in defined in the band.
        //!
        Polarization polarization(uint32_t channel) const;

        //!
        //! Compute a channel number from a frequency.
        //! @param [in] frequency Frequency in Hz.
        //! @return Channel number or zero on error.
        //!
        uint32_t channelNumber(uint64_t frequency) const;

        //!
        //! Compute an offset count from frequency (approximate if necessary)
        //! @param [in] frequency Frequency in Hz.
        //! @return Offset count (positive or negative).
        //!
        int32_t offsetCount(uint64_t frequency) const;

        //!
        //! Check if a frequency is in the HF band.
        //! @param [in] frequency Frequency in Hz.
        //! @param [in] strict If true, @a frequency must be strictly inside the allowed offset range
        //! of a channel. When false, only check that the frequency is inside the global band.
        //! @return True if the frequency is in the HF band.
        //!
        bool inBand(uint64_t frequency, bool strict = false) const;

        //!
        //! Return a human-readable description of a channel.
        //! @param [in] channel Channel number.
        //! @param [in] offset Channel offset count. Displayed only if non-zero.
        //! @return Channel description.
        //!
        UString description(uint32_t channel, int32_t offset) const;

    private:
        // Define a range of HF channels.
        class ChannelsRange
        {
        public:
            // Public members
            uint32_t     first_channel = 0;
            uint32_t     last_channel = 0;
            uint64_t     base_frequency = 0;
            uint64_t     channel_width = 0;
            int32_t      first_offset = 0;
            int32_t      last_offset = 0;
            uint64_t     offset_width = 0;
            Polarization even_polarity = POL_NONE;
            Polarization odd_polarity = POL_NONE;

            // Constructor.
            ChannelsRange() = default;

            // Lowest and highest frequency in range.
            uint64_t lowestFrequency(bool strict) const;
            uint64_t highestFrequency(bool strict) const;
            uint32_t channelNumber(uint64_t frequency) const;
            uint64_t frequency(uint32_t channel, int32_t offset) const;
        };

        // A list of channel ranges.
        typedef std::list<ChannelsRange> ChannelsRangeList;

        // Safe pointer to an HBBand object.
        // Not thread-safe since these objects are loaded once and remain constant.
        typedef SafePtr<HFBand, ts::null_mutex> HFBandPtr;

        // HFBand members.
        const UString     _band_name {};       // Type of HF band.
        uint32_t          _channel_count = 0;  // Number of channels in the band.
        UStringList       _regions {};         // List of applicable regions.
        ChannelsRangeList _channels {};        // Channel ranges, in order of channel numbers.

        // Default constructor (private only, use GetBand() from application).
        HFBand(const UString band_name) : _band_name(band_name) {}

        // Get the range of channels for a given channel number. Null _channels.end() on error.
        ChannelsRangeList::const_iterator getRange(uint32_t channel) const;

        // Create an HFBand from an XML element. Null pointer on error.
        static HFBandPtr FromXML(const xml::Element*);

        // An index in the repository of HFBand.
        class HFBandIndex: public StringifyInterface
        {
        public:
            UString band {};   // Lower case, no space.
            UString region {}; // Lower case, no space.

            // Constructor.
            HFBandIndex(const UString& b, const UString& r);

            // Operators for use as index.
            bool operator==(const HFBandIndex&) const;
            bool operator<(const HFBandIndex&) const;

            // StringifyInterface interface.
            virtual UString toString() const override;
        };

        // A map of HFBand by index.
        typedef std::map<HFBandIndex,HFBandPtr> HFBandMap;

        // The repository of HF bands.
        class HFBandRepository
        {
            TS_DECLARE_SINGLETON(HFBandRepository);
        public:
            // Load the repository if not already done. Return false on error.
            bool load(Report&);

            // Get an object from the repository.
            const HFBand* get(const UString& band, const UString& region, Report& report) const;

            // Get/set the default region.
            UString defaultRegion() const;
            void setDefaultRegion(const UString&);

            // List of available regions.
            const UStringList& allRegions() const { return _allRegions; }

            // List of available bands in a region.
            const UStringList allBands(const UString& region) const;

        private:
            mutable std::recursive_mutex _mutex {};
            UString     _default_region {};
            HFBandMap   _objects {};
            UStringList _allRegions {};
            HFBandPtr   _voidBand {};
        };
    };
}
