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
//!
//!  @file
//!  Definition of an HF frequency band (UHF, VHF).
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsEnumeration.h"
#include "tsMutex.h"
#include "tsSafePtr.h"
#include "tsCerrReport.h"
#include "tsSingletonManager.h"
#include "tsxml.h"

namespace ts {

    class HFBand;

    //!
    //! Safe pointer to an HFBand instance (thread-safe).
    //!
    typedef SafePtr<HFBand,Mutex> HFBandPtr;

    //!
    //! Definition of an HF frequency band (UHF, VHF).
    //! @ingroup hardware
    //!
    //! There is a repository of known UHF and VHF bands layout per country or region.
    //! This repository is read from an XML file. There is only one instance of HFBand
    //! per country or region.
    //! @see ts::HFBand::Factory()
    //!
    class TSDUCKDLL HFBand
    {
    public:
        //!
        //! Type of frequency band.
        //!
        enum BandType {
            VHF,  //!< VHF, Very High Frequency.
            UHF,  //!< UHF, Ultra High Frequency.
        };

        //!
        //! Get the default region.
        //! @param [in,out] report Where to report errors.
        //! @return The default region. This is the value of the parameter "default.region"
        //! in the TSDuck configuration file for the current application. If undefined in
        //! the configuration file, the default is "europe".
        //!
        static UString DefaultRegion(Report& report = CERR);

        //!
        //! Factory static method.
        //! @param [in] region Region of country name (not case sensitive).
        //! @param [in] type HF band type.
        //! @param [in,out] report Where to report errors.
        //! @return A safe pointer to the instance for the corresponding @a region.
        //! If the repository contains no known band for the region, return an empty object.
        //!
        static HFBandPtr Factory(const UString& region = UString(), BandType type = UHF, Report& report = CERR);

        //!
        //! Get the type of HF band.
        //! @return The type of HF band.
        //!
        BandType type() const { return _type; }

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
        //! @param [in] strength Signal strength in percent. Ignored if negative.
        //! @param [in] quality Signal quality in percent. Ignored if negative.
        //! @return Channel description.
        //!
        UString description(int channel, int offset, int strength = -1, int quality = -1) const;

    private:
        // Define a range of HF channels.
        class ChannelsRange
        {
        public:
            // Public members
            uint32_t first_channel;
            uint32_t last_channel;
            uint64_t base_frequency;
            uint64_t channel_width;
            int32_t  first_offset;
            int32_t  last_offset;
            uint64_t offset_width;

            // Constructor.
            ChannelsRange();

            // Lowest and highest frequency in range.
            uint64_t lowestFrequency(bool strict) const;
            uint64_t highestFrequency(bool strict) const;
            uint32_t channelNumber(uint64_t frequency) const;
            uint64_t frequency(uint32_t channel, int32_t offset) const;
        };

        // A list of channel ranges.
        typedef std::list<ChannelsRange> ChannelsRangeList;

        // HFBand members.
        const BandType    _type;          // Type of HF band.
        uint32_t          _channel_count; // Number of channels in the band.
        UStringList       _regions;       // List of applicable regions.
        ChannelsRangeList _channels;      // Channel ranges, in order of channel numbers.

        // Default constructor (private only, use Factory() from application).
        HFBand(BandType = UHF);

        // Get the range of channels for a given channel number. Null pointer on error.
        ChannelsRangeList::const_iterator getRange(uint32_t channel) const;

        // Create an HFBand from an XML element. Null pointer on error.
        static HFBandPtr FromXML(const xml::Element*);

        // An index in the repository of HFBand.
        class HFBandIndex: public StringifyInterface
        {
        public:
            const BandType type;
            const UString  region; // Lower case, no space.

            // Constructor.
            HFBandIndex(BandType, const UString&);

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
            HFBandPtr get(BandType type, const UString& region, Report& report) const;

            // Get the default region.
            UString defaultRegion() const { return _default_region; }

            // An enumeration object for BandType.
            const Enumeration bandTypeEnum;

        private:
            mutable Mutex _mutex;
            UString       _default_region;
            HFBandMap     _objects;
        };

        // Inaccessible operations.
        HFBand(const HFBand&) = delete;
        HFBand& operator=(const HFBand&) = delete;
    };
}
