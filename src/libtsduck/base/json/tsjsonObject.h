//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Implementation of a JSON object.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsjsonValue.h"

namespace ts {
    namespace json {
        //!
        //! Implementation of a JSON object.
        //! @ingroup json
        //!
        class TSDUCKDLL Object: public Value
        {
        public:
            //! Default constructor.
            Object() = default;

            // Implementation of ts::json::Value.
            virtual Type type() const override;
            virtual bool isObject() const override;
            virtual void print(TextFormatter& output) const override;
            virtual size_t size() const override;
            virtual const Value& value(const UString& name) const override;
            virtual Value& value(const UString& name, bool create = false, Type type = Type::Object) override;
            virtual ValuePtr valuePtr(const UString& name) override;
            virtual void remove(const UString& name) override;
            virtual ValuePtr extract(const UString& name) override;
            virtual void clear() override;
            virtual void getNames(UStringList& names) const override;
            virtual const Value& query(const UString& path) const override;
            virtual Value& query(const UString& path, bool create = false, Type type = Type::Object) override;

        protected:
            virtual void addValue(const UString& name, const ValuePtr& value) override;
            virtual void addInteger(const UString& name, int64_t value) override;
            virtual void addFloat(const UString& name, double value) override;
            virtual void addString(const UString& name, const UString& value) override;

        private:
            std::map<UString, ValuePtr> _fields {};

            // Split and validate a query path.
            static bool splitPath(const UString& path, UString& field, UString& next);
        };
    }
}
