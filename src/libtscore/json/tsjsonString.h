//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a JSON string.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"

namespace ts::json {
    //!
    //! Implementation of a JSON string.
    //! @ingroup libtscore json
    //!
    class TSCOREDLL String : public Value
    {
    public:
        //!
        //! Constructor.
        //! @param [in] value Initial string value.
        //!
        String(const UString& value = UString()) : _value(value) {}

        // Implementation of ts::json::Value.
        virtual Type type() const override;
        virtual bool isString() const override;
        virtual void print(TextFormatter& output) const override;
        virtual bool toBoolean(bool default_value = false) const override;
        virtual int64_t toInteger(int64_t default_value = 0) const override;
        virtual double toFloat(double default_value = 0.0) const override;
        virtual UString toString(const UString& default_value = UString()) const override;
        virtual size_t size() const override;
        virtual void clear() override;

    private:
        UString _value {};
    };
}
