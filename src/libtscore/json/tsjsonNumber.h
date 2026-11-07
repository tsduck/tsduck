//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a JSON number.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"
#include "tsIntegerUtils.h"

namespace ts::json {
    //!
    //! Implementation of a JSON number.
    //! @ingroup libtscore json
    //!
    class TSCOREDLL Number: public Value
    {
    public:
        //!
        //! Default constructor.
        //!
        Number() = default;

        //!
        //! Constructor with an integer value.
        //! @tparam T An integer  type.
        //! @param [in] value Initial value.
        //!
        template <typename T> requires std::integral<T>
        Number(T value) :
            _float(double(value)),
            _valid_integer(bound_check<int64_t>(value)),
            _integer(_valid_integer ? int64_t(value) : 0)
        {
        }

        //!
        //! Constructor with an integer or floating point value.
        //! @tparam T An integer or floating point type.
        //! @param [in] value Initial value.
        //! @param [in] force_invalid_integer If true, don't try to convert as an integer, consider it invalid.
        //!
        template <typename T> requires std::floating_point<T>
        Number(T value, bool force_invalid_integer = false) :
            _float(double(value)),
            _valid_integer(!force_invalid_integer && _float >= double(std::numeric_limits<int64_t>::min()) && _float <= double(std::numeric_limits<int64_t>::max())),
            _integer(_valid_integer ? int64_t(value) : 0)
        {
        }

        // Implementation of ts::json::Value.
        virtual Type type() const override;
        virtual bool isNumber() const override;
        virtual bool isInteger() const override;
        virtual void print(TextFormatter& output) const override;
        virtual bool toBoolean(bool default_value = false) const override;
        virtual int64_t toInteger(int64_t default_value = 0) const override;
        virtual double toFloat(double default_value = 0.0) const override;
        virtual UString toString(const UString& default_value = UString()) const override;
        virtual void clear() override;

    private:
        double  _float = 0.0;
        bool    _valid_integer = false; // if false, _float is ok but its value is not representable as an int64_t
        int64_t _integer = 0;
    };
}
