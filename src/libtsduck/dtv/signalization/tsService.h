//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
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
//----------------------------------------------------------------------------
//!
//!  @file
//!  Describe a DVB or ATSC service
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUString.h"
#include "tsVariable.h"
#include "tsStringifyInterface.h"
#include "tsTS.h"

namespace ts {
    //!
    //! Describe a DVB or ATSC service.
    //! @ingroup mpeg
    //!
    //! An instance of this class contains all possible properties of a
    //! DVB service. But all properties are optional. They may be set and
    //! cleared. Check the availability of a property before getting it.
    //!
    class TSDUCKDLL Service: public StringifyInterface
    {
    public:
        //!
        //! Default constructor.
        //!
        Service();

        //!
        //! Constructor using a service id.
        //! @param [in] id Service id.
        //!
        Service(uint16_t id);

        //!
        //! Constructor using a string description.
        //! @param [in] desc Service description string.
        //! If the string evaluates to an integer (decimal or hexa),
        //! this is a service id, otherwise this is a service name.
        //!
        Service(const UString& desc);

        //!
        //! Destructor.
        //!
        virtual ~Service() override;

        //!
        //! Reset using a string description.
        //! @param [in] desc Service description string.
        //! If the string evaluates to an integer (decimal or hexa),
        //! this is a service id, otherwise this is a service name.
        //!
        virtual void set(const UString& desc);

        //!
        //! Clear all fields.
        //!
        virtual void clear();

        //!
        //! Clear the "modified" indicator.
        //! This indicator is set whenever a field is updated with a different value.
        //!
        void clearModified() { _modified = false; }

        //!
        //! Check if any field was modified since the last call to clearModified().
        //! @return True if a field was modified since the last call to clearModified().
        //!
        bool isModified() const { return _modified; }

        // Implementation of StringifyInterface.
        virtual UString toString() const override;

        // Accessors to the properties are repeated, use macros.

        //!
        //! Define a service property accessors, class internal use only.
        //! @param type C++ type for the property.
        //! @param suffix Accessor methods suffix.
        //! @param field Internal class private field.
        //! @param defvalue Property default value if unset.
        //! @param fullname Explanatory description of the property.
        //! @hideinitializer
        //!
#define SERVICE_PROPERTY(type,suffix,field,defvalue,fullname)      \
        /** Check if the fullname is present.                   */ \
        /** @return True if the fullname is present.            */ \
        bool has##suffix() const {return field.set();}             \
        /** Clear the fullname.                                 */ \
        void clear##suffix()                                       \
            {_modified = _modified || field.set(); field.clear();} \
        /** Get the fullname.                                   */ \
        /** @return The fullname or defvalue if unset.          */ \
        type get##suffix() const                                   \
            {return field.set() ? field.value() : type(defvalue);}

        //!
        //! Define an integer service property accessors, class internal use only.
        //! @param type C++ type for the property.
        //! @param suffix Accessor methods suffix.
        //! @param field Internal class private field.
        //! @param defvalue Property default value if unset.
        //! @param fullname Explanatory description of the property.
        //! @hideinitializer
        //!
#define SERVICE_PROPERTY_INT(type,suffix,field,defvalue,fullname)     \
        SERVICE_PROPERTY(type, suffix, field, defvalue, fullname)     \
        /** Set the fullname.                                      */ \
        /** @param [in] value The fullname.                        */ \
        void set##suffix(type value)                                  \
            {_modified = _modified || field != value; field = value;} \
        /** Check if the fullname has a given value.               */ \
        /** @param [in] value The fullname to check.               */ \
        /** @return True if the fullname is equal to @a value.     */ \
        bool has##suffix(type value) const                            \
            {return field.set() && field == value;}

        //!
        //! Define a string service property accessors, class internal use only.
        //! @param suffix Accessor methods suffix.
        //! @param field Internal class private field.
        //! @param fullname Explanatory description of the property.
        //! @hideinitializer
        //!
#define SERVICE_PROPERTY_STRING(suffix,field,fullname)                \
        SERVICE_PROPERTY(UString, suffix, field, UString(), fullname) \
        /** Set the fullname.                                      */ \
        /** @param [in] value The fullname.                        */ \
        void set##suffix(const UString& value)                        \
            {_modified = _modified || field != value; field = value;} \
        /** Check if the fullname has a given value.               */ \
        /** @param [in] value The fullname to check.               */ \
        /** @return True if the fullname is similar to @a value,   */ \
        /** case insensitive and ignoring blanks.                  */ \
        bool has##suffix(const UString& value) const                  \
            {return field.set() && value.similar(field.value());}

        SERVICE_PROPERTY_INT(uint16_t, Id,            _id,             0,        Service Id)
        SERVICE_PROPERTY_INT(uint16_t, TSId,          _tsid,           0,        Transport Stream Id)
        SERVICE_PROPERTY_INT(uint16_t, ONId,          _onid,           0,        Original Network Id)
        SERVICE_PROPERTY_INT(uint16_t, LCN,           _lcn,            0,        Logical Channel Number)
        SERVICE_PROPERTY_INT(PID,      PMTPID,        _pmt_pid,        PID_NULL, PMT PID)
        SERVICE_PROPERTY_INT(uint8_t,  TypeDVB,       _type_dvb,       0,        DVB service type (as declared in service_descriptor))
        SERVICE_PROPERTY_INT(uint8_t,  TypeATSC,      _type_atsc,      0,        ATSC service type (as declared in TVCT or CVCT))
        SERVICE_PROPERTY_INT(uint8_t,  RunningStatus, _running_status, 0,        Running status (as declared in the SDT))
        SERVICE_PROPERTY_INT(bool,     EITsPresent,   _eits_present,   false,    EIT schedule present (as declared in the SDT))
        SERVICE_PROPERTY_INT(bool,     EITpfPresent,  _eitpf_present,  false,    EIT present/following present (as declared in the SDT))
        SERVICE_PROPERTY_INT(bool,     CAControlled,  _ca_controlled,  false,    CA-controlled (as declared in the SDT))
        SERVICE_PROPERTY_INT(uint16_t, MajorIdATSC,   _major_id_atsc,  0,        ATSC major id (as declared in TVCT or CVCT))
        SERVICE_PROPERTY_INT(uint16_t, MinorIdATSC,   _minor_id_atsc,  0,        ATSC major id (as declared in TVCT or CVCT))

        SERVICE_PROPERTY_STRING(Name,     _name,     Service Name)
        SERVICE_PROPERTY_STRING(Provider, _provider, Provider Name)

#undef SERVICE_PROPERTY
#undef SERVICE_PROPERTY_INT
#undef SERVICE_PROPERTY_STRING

        //!
        //! List of possible fields a Service may have set.
        //! Can be used as bitfield.
        //!
        enum ServiceField : uint32_t {
            ID           = 0x0001,  //!< Service id.
            TSID         = 0x0002,  //!< Transport stream id.
            ONID         = 0x0004,  //!< Original network id.
            PMT_PID      = 0x0008,  //!< PMT PID.
            LCN          = 0x0010,  //!< Logical channel number.
            TYPE_DVB     = 0x0020,  //!< DVB service type (as defined in service_descriptor).
            NAME         = 0x0040,  //!< Service name.
            PROVIDER     = 0x0080,  //!< Provider name.
            EITS         = 0x0100,  //!< EIT schedule present (as declared in the SDT).
            EITPF        = 0x0200,  //!< EIT present/following present (as declared in the SDT).
            CA           = 0x0400,  //!< CA-controlled (as declared in the SDT).
            RUNNING      = 0x0800,  //!< Running status (as declared in the SDT).
            TYPE_ATSC    = 0x1000,  //!< ATSC service type (as defined in TVCT or CVCT).
            MAJORID_ATSC = 0x2000,  //!< ATSC major id (as declared in TVCT or CVCT)).
            MINORID_ATSC = 0x4000,  //!< ATSC minor id (as declared in TVCT or CVCT)).
        };

        //!
        //! Get the list of fields which are set in a Service.
        //! @return The list of fields which are set in a Service as
        //! an or'ed mask of ServiceField values.
        //!
        uint32_t getFields() const;

        //!
        //! Check if a service matches a string identification.
        //! @param [in] ident Service identification, either an integer (service id) or service name.
        //! @param [in] exact_match If true, the service name must be exactly identical to @a ident.
        //! If it is false, the search is case-insensitive and blanks are ignored.
        //! @return True if the service matches @a ident.
        //!
        bool match(const UString& ident, bool exact_match = false) const;

        //!
        //! Sorting criterion method, used by std::sort().
        //! Sort order: LCN, ONId, TSId, Id, name, provider, type, PMT PID.
        //!
        //! If both objects have a given field set, sort according to this field.
        //! If only one object has this field set, it comes first. If none of the
        //! two objects have this field set, use to next criterion.
        //!
        //! @param [in] s1 First service.
        //! @param [in] s2 First service.
        //! @return True is @a s1 is logically less than @a s2, false otherwise.
        //!
        static bool Sort1(const Service& s1, const Service& s2);

        //!
        //! Sorting criterion method, used by std::sort().
        //! Sort order: name, provider, LCN, ONId, TSId, Id, type, PMT PID.
        //!
        //! If both objects have a given field set, sort according to this field.
        //! If only one object has this field set, it comes first. If none of the
        //! two objects have this field set, use to next criterion.
        //!
        //! @param [in] s1 First service.
        //! @param [in] s2 First service.
        //! @return True is @a s1 is logically less than @a s2, false otherwise.
        //!
        static bool Sort2(const Service& s1, const Service& s2);

        //!
        //! Sorting criterion method, used by std::sort().
        //! Sort order: ONId, TSId, Id, type, name, provider, LCN, PMT PID.
        //!
        //! If both objects have a given field set, sort according to this field.
        //! If only one object has this field set, it comes first. If none of the
        //! two objects have this field set, use to next criterion.
        //!
        //! @param [in] s1 First service.
        //! @param [in] s2 First service.
        //! @return True is @a s1 is logically less than @a s2, false otherwise.
        //!
        static bool Sort3(const Service& s1, const Service& s2);

        //!
        //! Display a container of services, one line per service.
        //! @tparam ITERATOR An iterator class in the container.
        //! @param [in,out] strm Output text stream.
        //! @param [in] margin The string to print as left margin.
        //! @param [in] begin Iterator to the first object to display.
        //! @param [in] end Iterator after the last object to display.
        //! @param [in] header If true, display a header line first.
        //! @return A reference to @a strm.
        //!
        template<class ITERATOR>
        static std::ostream& Display(std::ostream& strm,
                                     const UString& margin,
                                     const ITERATOR& begin,
                                     const ITERATOR& end,
                                     bool header = true);

        //!
        //! Display a container of services, one line per service.
        //! @tparam CONTAINER  A container class.
        //! @param [in,out] strm Output text stream.
        //! @param [in] margin The string to print as left margin.
        //! @param [in] container Container of services to display.
        //! @param [in] header If true, display a header line first.
        //! @return A reference to @a strm.
        //!
        template<class CONTAINER>
        static std::ostream& Display(std::ostream& strm, const UString& margin, const CONTAINER& container, bool header = true)
        {
            return Display(strm, margin, container.begin(), container.end(), header);
        }

    private:
        bool               _modified;
        Variable<uint16_t> _id;
        Variable<uint16_t> _tsid;
        Variable<uint16_t> _onid;
        Variable<PID>      _pmt_pid;
        Variable<uint16_t> _lcn;
        Variable<uint8_t>  _type_dvb;
        Variable<uint8_t>  _type_atsc;
        Variable<UString>  _name;
        Variable<UString>  _provider;
        Variable<bool>     _eits_present;
        Variable<bool>     _eitpf_present;
        Variable<bool>     _ca_controlled;
        Variable<uint8_t>  _running_status;
        Variable<uint16_t> _major_id_atsc;
        Variable<uint16_t> _minor_id_atsc;
    };

    // Containers
    typedef std::vector<Service> ServiceVector;  //!< Vector of DVB services.
    typedef std::list<Service> ServiceList;      //!< List of DVB services.
    typedef std::set<Service> ServiceSet;        //!< Set of DVB services.
}

#include "tsServiceTemplate.h"
