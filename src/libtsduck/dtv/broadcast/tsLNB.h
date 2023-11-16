//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Description of a Low-Noise Block (LNB) converter in a satellite dish.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStringifyInterface.h"
#include "tsUString.h"
#include "tsCerrReport.h"
#include "tsModulation.h"
#include "tsReport.h"
#include "tsSafePtr.h"
#include "tsSingleton.h"
#include "tsxml.h"

namespace ts {
    //!
    //! Description of a Low-Noise Block (LNB) converter in a satellite dish.
    //! @ingroup hardware
    //!
    //! The satellite carrier frequency is used to carry the signal from the
    //! satellite to the dish. This value is public and is stored in the NIT
    //! for instance. The intermediate frequency is used to carry the signal
    //! from the dish's LNB to the receiver. The way this frequency is
    //! computed depends on the characteristics of the LNB. The intermediate
    //! frequency is the one that is used by the tuner in the satellite
    //! receiver.
    //!
    //! Note: all frequencies are in Hz in parameters.
    //!
    class TSDUCKDLL LNB : public StringifyInterface
    {
    public:
        //!
        //! Default constructor.
        //! The object is initially invalid.
        //!
        LNB() = default;

        //!
        //! Constructor from an LNB name.
        //! @param [in] name LNB name of alias as found in file tsduck.lnbs.xml.
        //! Can also be a full specification in legacy format (frequencies in MHz):
        //! - "freq" if the LNB has no high band.
        //! - "low,high,switch" if the LNB has a high band.
        //! @param [in,out] report Where to log errors.
        //!
        LNB(const UString& name, Report& report = CERR);

        //!
        //! Constructor from a simple legacy LNB without high band.
        //! @param [in] frequency Low frequency.
        //!
        LNB(uint64_t frequency);

        //!
        //! Constructor from a legacy LNB with low and high band.
        //! @param [in] low_frequency Low frequency.
        //! @param [in] high_frequency High frequency.
        //! @param [in] switch_frequency Switch frequency.
        //!
        LNB(uint64_t low_frequency, uint64_t high_frequency, uint64_t switch_frequency);

        //!
        //! Get a list of all available LNB's from the configuration file.
        //! @param [in,out] report Where to report errors.
        //! @return The list of all available LNB's and aliases.
        //!
        static UStringList GetAllNames(Report& report = CERR);

        //!
        //! Get the official name of the LNB.
        //! @return The official name of the LNB.
        //!
        UString name() const { return _name; }

        //!
        //! Convert the LNB object to a string.
        //! @return A string representing the LNB. This may be different from name().
        //! If the official name contains spaces or other not convenient characters
        //! to use on the command line, and a more convenient alias is defined in the
        //! configuration file, then this alias is used.
        //! @see StringifyInterface
        //!
        virtual UString toString() const override;

        //!
        //! Check if valid (typically after initializing or converting from string).
        //! @return True if valid.
        //!
        bool isValid() const { return !_bands.empty(); }

        //!
        //! Check if the LNB is polarization-controlled.
        //! With such LNB's, the satellite frequencies are transposed in different bands
        //! depending on the polarity. Also, they use "stacked" transposition: the transposed
        //! bands don't overlap and no tone/voltage/DiSEqC command is needed.
        //! @return True if the LNB is polarization-controlled.
        //!
        bool isPolarizationControlled() const;

        //!
        //! Get the number of frequency bands in the LNB.
        //! @return The number of frequency bands in the LNB.
        //!
        size_t bandsCount() const { return _bands.size(); }

        //!
        //! Get the legacy "low oscillator frequency" value.
        //! @return The legacy "low oscillator frequency" or zero if there no equivalent.
        //!
        uint64_t legacyLowOscillatorFrequency() const;

        //!
        //! Get the legacy "high oscillator frequency" value.
        //! @return The legacy "high oscillator frequency" or zero if there no equivalent.
        //!
        uint64_t legacyHighOscillatorFrequency() const;

        //!
        //! Get the legacy "switch frequency" value.
        //! @return The legacy "switch frequency" or zero if there no equivalent.
        //!
        uint64_t legacySwitchFrequency() const;

        //!
        //! Description of the required transposition for a given satellite frequency and polarization.
        //!
        class TSDUCKDLL Transposition
        {
        public:
            Transposition() = default;            //!< Constructor.
            uint64_t satellite_frequency = 0;     //!< Satellite frequency.
            uint64_t intermediate_frequency = 0;  //!< Intermediate frequency.
            uint64_t oscillator_frequency = 0;    //!< Oscillator frequency.
            bool     stacked = false;             //!< All transpositions are "stacked", no need to send a command to the dish.
            size_t   band_index = 0;              //!< Band index to switch to (e.g. 0 and 1 for low and high band of a universal LNB).
        };

        //!
        //! Compute the intermediate frequency and transposition from a satellite carrier frequency.
        //! @param [out] transposition Returned transposition information.
        //! @param [in] satellite_frequency Satellite carrier frequency in Hz.
        //! @param [in] polarity Carrier polarity. Used only on polarization-controlled LNB's. These LNB's
        //! typically transpose different polarizations in different bands of intermediate frequencies.
        //! @param [in,out] report Where to log errors.
        //! @return True on success, false on error.
        //! Return zero on error (invalid LNB, frequency out or range).
        //!
        bool transpose(Transposition& transposition, uint64_t satellite_frequency, Polarization polarity, Report& report = CERR) const;

        //!
        //! Set the LNB to the specified type of LNB.
        //! @param [in] name LNB name of alias as found in file tsduck.lnbs.xml.
        //! Can also be a full specification in legacy format (frequencies in MHz):
        //! - "freq" if the LNB has no high band.
        //! - "low,high,switch" if the LNB has a high band.
        //! @param [in,out] report Where to log errors.
        //! @return True on success, false on error. In case of error, an error
        //! is displayed and the LNB object is marked as invalid.
        //!
        bool set(const UString& name, Report& report = CERR);

        //!
        //! Set values of a simple legacy LNB without high band.
        //! @param [in] frequency Low frequency.
        //!
        void set(uint64_t frequency);

        //!
        //! Set values of a legacy LNB with low and high band.
        //! @param [in] low_frequency Low frequency.
        //! @param [in] high_frequency High frequency.
        //! @param [in] switch_frequency Switch frequency.
        //!
        void set(uint64_t low_frequency, uint64_t high_frequency, uint64_t switch_frequency);

    private:
        // One frequency band, as supported by the LNB.
        class Band
        {
        public:
            Band() = default;                  // Constructor.
            uint64_t     low = 0;              // Lower bound of frequency band.
            uint64_t     high = 0;             // Higher bound of frequency band.
            uint64_t     oscillator = 0;       // Oscillator frequency (base of transposition).
            uint64_t     switch_freq = 0;      // Switch frequency (to next band).
            Polarization polarity = POL_NONE;  // Polarity of this band (POL_NONE if not polarity-driven).
        };

        // LNB private members.
        UString           _name {};     // Official or rebuilt name.
        UString           _alias {};    // Convenient alias, safe for command line use.
        std::vector<Band> _bands {};    // All supported frequency bands.

        // Safe pointer to an LNB object.
        // Not thread-safe since these objects are loaded once and remain constant.
        typedef SafePtr<LNB, ts::null_mutex> LNBPtr;

        // The repository of known LNB's.
        class LNBRepository
        {
            TS_DECLARE_SINGLETON(LNBRepository);
        public:
            // Get an LNB by name or alias from the repository (default LNB when name is empty).
            // Return null pointer when not found.
            const LNB* get(const UString& name, Report& report);

            // List of available LNB names. Return a constant reference to a constant object.
            const UStringList& allNames(Report& report);

        private:
            mutable std::mutex       _mutex {};
            LNBPtr                   _default_lnb {};
            std::map<UString,LNBPtr> _lnbs {};
            UStringList              _names {};

            // Convert a name to an index in LNB map.
            static UString ToIndex(const UString& name);

            // Load the repository if not already done. Return false on error.
            bool load(Report&);

            // Get name attribute of an <lnb> or <alias> element. Return false on error.
            // Full name is added in _names. Normalized name is added in inames parameter.
            bool getNameAttribute(const xml::Element* node, UString& name, UStringList& index_names);
        };
    };
}
