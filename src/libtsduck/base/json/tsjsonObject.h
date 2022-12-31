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
            //!
            //! Constructor.
            //!
            Object() : _fields() {}

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
            virtual void add(const UString& name, const ValuePtr& value) override;
            virtual void add(const UString& name, int64_t value) override;
            virtual void add(const UString& name, const UString& value) override;
            virtual void clear() override;
            virtual void getNames(UStringList& names) const override;
            virtual const Value& query(const UString& path) const override;
            virtual Value& query(const UString& path, bool create = false, Type type = Type::Object) override;

        private:
            std::map<UString, ValuePtr> _fields;

            // Split and validate a query path.
            static bool splitPath(const UString& path, UString& field, UString& next);
        };
    }
}
