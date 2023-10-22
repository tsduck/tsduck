//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a JSON null literal.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"

namespace ts {
    namespace json {
        //!
        //! Implementation of a JSON null literal.
        //! @ingroup json
        //!
        class TSDUCKDLL Null : public Value
        {
        public:
            //! Default constructor.
            Null() = default;

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isNull() const override;
            virtual void print(TextFormatter& output) const override;
        };

        //!
        //! A general-purpose null JSON value.
        //! This object is not marked as "const" but, like any Null value,
        //! all modificaton operations do nothing.
        //!
        extern Null NullValue;
    }
}
