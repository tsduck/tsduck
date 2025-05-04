//-----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//-----------------------------------------------------------------------------
//!
//!  @file
//!  Encapsulation of Linux DVB property lists.
//!
//-----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"

#include "tsBeforeStandardHeaders.h"
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <linux/version.h>
#include "tsAfterStandardHeaders.h"

//!
//! @hideinitializer
//! On Linux systems, identify the Linux DVB API version in one value.
//! Example: TS_DVB_API_VERSION is 503 for DVB API version 5.3.
//!
#define TS_DVB_API_VERSION ((DVB_API_VERSION * 100) + DVB_API_VERSION_MINOR)

namespace ts {
    //!
    //! Encapsulation of Linux DVB property lists.
    //! @ingroup libtsduck unix
    //!
    class TSDUCKDLL DTVProperties
    {
        TS_NOCOPY(DTVProperties);
    public:
        //!
        //! Default constructor.
        //!
        DTVProperties();

        //!
        //! Destructor.
        //!
        virtual ~DTVProperties();

        //!
        //! Get the number of properties in the buffer.
        //! @return The number of properties in the buffer.
        //!
        size_t count() const { return size_t(_prop_head.num); }

        //!
        //! Clear all previously added commands.
        //!
        void clear() { _prop_head.num = 0; }

        //!
        //! Add a new property.
        //! @param [in] cmd Command code.
        //! @param [in] data Optional command data.
        //! @return The index in property buffer.
        //!
        size_t add(uint32_t cmd, uint32_t data = -1);

        //!
        //! Add a new property if a variable is set.
        //! @param [in] cmd Command code.
        //! @param [in] data Optional command data.
        //!
        template <typename ENUM> requires std::integral<ENUM> || std::is_enum_v<ENUM>
        void addVar(uint32_t cmd, const std::optional<ENUM>& data)
        {
            if (data.has_value()) {
                add(cmd, uint32_t(data.value()));
            }
        }

        //!
        //! Add a new property to get statistics.
        //! @param [in] cmd Command code.
        //! @return The index in property buffer.
        //!
        size_t addStat(uint32_t cmd);

        //!
        //! Search a property in the buffer.
        //! @param [in] cmd Command code.
        //! @return The index of @a cmd in buffer or count() if not found.
        //!
        size_t search(uint32_t cmd) const;

        //!
        //! Get the value of a property in the buffer.
        //! @param [in] cmd Command code.
        //! @return The data value of @a cmd in buffer or @link UNKNOWN @endlink if not found.
        //!
        uint32_t getByCommand(uint32_t cmd) const;

        //!
        //! Get the value of the property at a specified index.
        //! @param [in] index Index in buffer.
        //! @return The data value at @a index or @link UNKNOWN @endlink if out of range.
        //!
        uint32_t getByIndex(size_t index) const;

        //!
        //! Get the value of a statistics property in the buffer.
        //! @param [out] value Statistics value.
        //! @param [out] scale Representation of the value.
        //! @param [in] cmd Command code.
        //! @param [in] layer Statistics layer. 0 in most case, 1..3 for ISDB sub-layers.
        //! @return True on success, false on error or not available.
        //!
        bool getStatByCommand(int64_t& value, ::fecap_scale_params& scale, uint32_t cmd, size_t layer) const;

        //!
        //! Get the multiple values of a property in the buffer.
        //! To be used with properties which return a set of integer values.
        //! @tparam INT An integer or enum type, any size, signed or unsigned.
        //! @param [out] values A set receiving all integer values.
        //! @param [in] cmd Command code.
        //!
        template <typename INT> requires std::integral<INT> || std::is_enum_v<INT>
        void getValuesByCommand(std::set<INT>& values, uint32_t cmd) const;

        //!
        //! Get the multiple values of a property at a specified index.
        //! To be used with properties which return a set of integer values.
        //! @tparam INT An integer or enum type, any size, signed or unsigned.
        //! @param [out] values A set receiving all integer values.
        //! @param [in] index Index in buffer.
        //!
        template <typename INT> requires std::integral<INT> || std::is_enum_v<INT>
        void getValuesByIndex(std::set<INT>& values, size_t index) const;

        //!
        //! Get the address of the @c dtv_properties structure for @c ioctl() call.
        //! @return The address of the @c dtv_properties structure.
        //!
        const ::dtv_properties* getIoctlParam() const { return &_prop_head; }

        //!
        //! Get the address of the @c dtv_properties structure for @c ioctl() call.
        //! @return The address of the @c dtv_properties structure.
        //!
        ::dtv_properties* getIoctlParam() { return &_prop_head; }

        //!
        //! Returned value for unknown data.
        //!
        static constexpr uint32_t UNKNOWN = ~0U;

        //!
        //! Display the content of the object (for debug purpose).
        //! @param [in,out] report Where to display the content.
        //! @param [in] severity Severity level of messages (typically a debug level).
        //!
        void report(Report& report, int severity) const;

        //!
        //! Display the statistics content of the object (for debug purpose).
        //! @param [in,out] report Where to display the content.
        //! @param [in] severity Severity level of messages (typically a debug level).
        //!
        void reportStat(Report& report, int severity) const;

        //!
        //! Return the name of a command.
        //! @param [in] cmd Command code.
        //! @return A name for @a cmd or a null pointer if unknown.
        //!
        static const char* CommandName(uint32_t cmd) { return DTVNames::Instance().name(cmd); }

    private:
        // Private members:
        ::dtv_property   _prop_buffer[DTV_IOCTL_MAX_MSGS];
        ::dtv_properties _prop_head {0, _prop_buffer};

        // A singleton holding all DTV_ names.
        class DTVNames
        {
            TS_SINGLETON(DTVNames);
        public:
            const char* name(uint32_t cmd);
        private:
            std::map<uint32_t,const char*> _names {};
            void reg(const char* name, const char* value);
        };
    };
}


//----------------------------------------------------------------------------
// Template definitions.
//----------------------------------------------------------------------------

template <typename INT> requires std::integral<INT> || std::is_enum_v<INT>
void ts::DTVProperties::getValuesByCommand(std::set<INT>& values, uint32_t cmd) const
{
    values.clear();
    for (size_t i = 0; i < size_t(_prop_head.num); i++) {
        if (_prop_buffer[i].cmd == cmd) {
            getValuesByIndex(values, i);
            break;
        }
    }
}

template <typename INT> requires std::integral<INT> || std::is_enum_v<INT>
void ts::DTVProperties::getValuesByIndex(std::set<INT>& values, size_t index) const
{
    values.clear();
    if (index < size_t(_prop_head.num)) {
        assert(sizeof(_prop_buffer[index].u.buffer.data[0]) == 1);
        const size_t count = std::min<size_t>(sizeof(_prop_buffer[index].u.buffer.data), _prop_buffer[index].u.buffer.len);
        for (size_t i = 0; i < count; ++i) {
            values.insert(INT(_prop_buffer[index].u.buffer.data[i]));
        }
    }
}
