//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  @ingroup hardware
//!  Definition for transmission delivery systems.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsStandards.h"
#include "tsEnumeration.h"
#include "tsStringifyInterface.h"

#if defined(TS_LINUX)
    #include "tsBeforeStandardHeaders.h"
    #include <linux/dvb/frontend.h>
    #include <linux/version.h>
    #include "tsAfterStandardHeaders.h"
#endif

namespace ts {
    //!
    //! Delivery systems.
    //! Not all delivery systems are supported by TSDuck.
    //! Linux and Windows may also support different systems.
    //!
    enum DeliverySystem {
#if defined(TS_LINUX) && !defined(DOXYGEN)
        DS_UNDEFINED     = ::SYS_UNDEFINED,
        DS_DVB_S         = ::SYS_DVBS,
        DS_DVB_S2        = ::SYS_DVBS2,
        DS_DVB_S_TURBO   = ::SYS_TURBO,
        DS_DVB_T         = ::SYS_DVBT,
        DS_DVB_T2        = ::SYS_DVBT2,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
        DS_DVB_C_ANNEX_A = ::SYS_DVBC_ANNEX_AC,
#else
        DS_DVB_C_ANNEX_A = ::SYS_DVBC_ANNEX_A,
#endif
        DS_DVB_C_ANNEX_B = ::SYS_DVBC_ANNEX_B,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0)
        DS_DVB_C_ANNEX_C = -11,
#else
        DS_DVB_C_ANNEX_C = ::SYS_DVBC_ANNEX_C,
#endif
        DS_DVB_C2        = -10,
        DS_DVB_H         = ::SYS_DVBH,
        DS_ISDB_S        = ::SYS_ISDBS,
        DS_ISDB_T        = ::SYS_ISDBT,
        DS_ISDB_C        = ::SYS_ISDBC,
        DS_ATSC          = ::SYS_ATSC,
        DS_ATSC_MH       = ::SYS_ATSCMH,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
        DS_DTMB          = ::SYS_DMBTH,
#else
        DS_DTMB          = ::SYS_DTMB,
#endif
        DS_CMMB          = ::SYS_CMMB,
        DS_DAB           = ::SYS_DAB,
        DS_DSS           = ::SYS_DSS,
#else
        DS_UNDEFINED,      //!< Undefined.
        DS_DVB_S,          //!< DVB-S.
        DS_DVB_S2,         //!< DVB-S2.
        DS_DVB_S_TURBO,    //!< DVB-S Turbo.
        DS_DVB_T,          //!< DVB-T.
        DS_DVB_T2,         //!< DVB-T2.
        DS_DVB_C_ANNEX_A,  //!< DVB-C ITU-T J.83 Annex A.
        DS_DVB_C_ANNEX_B,  //!< DVB-C ITU-T J.83 Annex B.
        DS_DVB_C_ANNEX_C,  //!< DVB-C ITU-T J.83 Annex C.
        DS_DVB_C2,         //!< DVB-C2.
        DS_DVB_H,          //!< DVB-H (deprecated).
        DS_ISDB_S,         //!< ISDB-S.
        DS_ISDB_T,         //!< ISDB-T.
        DS_ISDB_C,         //!< ISDB-C.
        DS_ATSC,           //!< ATSC.
        DS_ATSC_MH,        //!< ATSC-M/H (mobile handheld).
        DS_DTMB,           //!< DTMB Terrestrial.
        DS_CMMB,           //!< CMMB Terrestrial.
        DS_DAB,            //!< DAB (digital audio).
        DS_DSS,            //!< DSS Satellite.
#endif
        DS_DVB_C = DS_DVB_C_ANNEX_A, //!< DVB-C, synonym for DVB-C Annex A.
    };

    //!
    //! Enumeration description of ts::DeliverySystem.
    //!
    TSDUCKDLL extern const Enumeration DeliverySystemEnum;

    //!
    //! A subset of ts::DeliverySystem describing types of tuners.
    //!
    enum TunerType {
        TT_UNDEFINED = DS_UNDEFINED,
        TT_DVB_S     = DS_DVB_S,
        TT_DVB_T     = DS_DVB_T,
        TT_DVB_C     = DS_DVB_C,
        TT_ISDB_S    = DS_ISDB_S,
        TT_ISDB_T    = DS_ISDB_T,
        TT_ISDB_C    = DS_ISDB_C,
        TT_ATSC      = DS_ATSC,
    };

    //!
    //! Enumeration description for the subset of ts::DeliverySystem describing types of tuners.
    //!
    TSDUCKDLL extern const Enumeration TunerTypeEnum;

    //!
    //! Get the tuner type of a delivery system.
    //! @param [in] system Delivery system.
    //! @return Corresponding tuner type or DS_UNDEFINED if there is no corresponding tuner type.
    //!
    TSDUCKDLL TunerType TunerTypeOf(DeliverySystem system);

    //!
    //! Get the list of standards for a delivery system.
    //! @param [in] system Delivery system.
    //! @return Corresponding standards.
    //!
    TSDUCKDLL Standards StandardsOf(DeliverySystem system);

    //!
    //! Check if a delivery system is a satellite one.
    //! This can be used to check if dish manipulations are required.
    //! @param [in] sys The delivery system to check.
    //! @return True if @a sys is a satellite system, false otherwise.
    //!
    TSDUCKDLL bool IsSatelliteDelivery(DeliverySystem sys);

    //!
    //! Check if a delivery system is a terrestrial one.
    //! This can be used to validate the use of UHD and VHF bands.
    //! @param [in] sys The delivery system to check.
    //! @return True if @a sys is a terrestrial system, false otherwise.
    //!
    TSDUCKDLL bool IsTerrestrialDelivery(DeliverySystem sys);

    //!
    //! An ordered list of delivery system values (ts::DeliverySystem).
    //!
    typedef std::list<DeliverySystem> DeliverySystemList;

    // GCC: error: base class 'class std::set<ts::DeliverySystem>' has accessible non-virtual destructor
    // This is harmless here since the subclass does not allocate own resources.
    TS_PUSH_WARNING()
    TS_GCC_NOWARNING(non-virtual-dtor)

    //!
    //! A set of delivery system values (ts::DeliverySystem).
    //! Typically used to indicate the list of standards which are supported by a tuner.
    //!
    class TSDUCKDLL DeliverySystemSet : public std::set<DeliverySystem>, public StringifyInterface
    {
        TS_DEFAULT_COPY_MOVE(DeliverySystemSet);
    public:
        //!
        //! Explicit reference to superclass.
        //!
        typedef std::set<DeliverySystem> SuperClass;

        //! Check if a delivery system is present in the set.
        //! @param [in] ds The delivery system to check.
        //! @return True if the specified delivery system is present.
        //!
        bool contains(DeliverySystem ds) const { return find(ds) != end(); }

        //!
        //! Get the "preferred" delivery system in the set.
        //! This can be used as default delivery system for a tuner.
        //! @return The "preferred" delivery system in the set.
        //!
        DeliverySystem preferred() const;

        //!
        //! Return the content of the set in decreasing order of preference.
        //! @return A list of all delivery systems in the set, in decreasing order of preference.
        //!
        DeliverySystemList toList() const;

        //!
        //! Get the list of standards for the set of delivery systems.
        //! @return Corresponding standards.
        //!
        Standards standards() const;

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

        //! @cond nodoxygen
        // Trampolines to superclass constructors.
        DeliverySystemSet() = default;
        DeliverySystemSet(std::initializer_list<value_type> init) : SuperClass(init) {}
        template<class InputIt> DeliverySystemSet(InputIt first, InputIt last) : SuperClass(first, last) {}
        //! @endcond

    private:
        // List of delivery systems, from most preferred to least preferred.
        // This list is used to find the default delivery system of a tuner and
        // to build the list of supported delivery systems in order of preference.
        static const ts::DeliverySystemList _preferred_order;
    };

    TS_POP_WARNING()
}
