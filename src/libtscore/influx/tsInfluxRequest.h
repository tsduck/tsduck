//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Client request for an InfluxDB server.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsWebRequest.h"
#include "tsInfluxArgs.h"
#include "tsTime.h"

namespace ts {
    //!
    //! Client request for an InfluxDB server.
    //! @ingroup libtscore net
    //! @see https://docs.influxdata.com/influxdb/v2/
    //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
    //! @see https://docs.influxdata.com/influxdb/v2/api/v2/#operation/PostWrite
    //!
    class TSCOREDLL InfluxRequest : public WebRequest
    {
        TS_NOBUILD_NOCOPY(InfluxRequest);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //! @param [in] args The connection information to the InfluxDB server.
        //! A reference is kept in this object.
        //!
        InfluxRequest(Report& report, const InfluxArgs& args);

        //!
        //! Destructor.
        //!
        virtual ~InfluxRequest() override;

        //!
        //! Start building a request to the InfluxDB server.
        //! @param [in] timestamp Value of the timestamp for that request.
        //!
        void start(Time timestamp);

        //!
        //! Add a line in the request being built, with one single integer value.
        //! @param [in] measurement The name of the measurement.
        //! @param [in] tags Comma-separated list of tags "name=value". The names and values
        //! must be compatible with the InfluxDB line protocol (use ToKey() if necessary.
        //! @param [in] value The measurement value. The field name is implicitly "value".
        //!
        template <typename INT_T> requires std::integral<INT_T>
        void add(const UString& measurement, const UString& tags, INT_T value)
        {
            add(measurement, tags, UString::Format(u"value=%d", value));
        }

        //!
        //! Add a line in the request being built, with generic value fields.
        //! @param [in] measurement The name of the measurement.
        //! @param [in] tags Comma-separated list of tags "name=value". The names and values
        //! must be compatible with the InfluxDB line protocol (use ToKey() if necessary.
        //! @param [in] fields The measurement fields. The field names and values must be
        //! compatible with the InfluxDB line protocol.
        //!
        void add(const UString& measurement, const UString& tags, const UString& fields);

        //!
        //! Get the current content of the request being built.
        //! For debug purpose only.
        //! @return A constant reference to a string object containing the currnet content.
        //!
        const UString& currentContent() const { return _builder; }

        //!
        //! Complete the request being built and send it to the InfluxDB server.
        //! @return True on success, false on error.
        //!
        bool send();

        //!
        //! Escape characters in a string to be used as measurement.
        //! @param [in] name The name to transform.
        //! @return The transformed string with escaped special characters.
        //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
        //!
        static UString ToMeasurement(const UString& name) { return Escape(name, u", \\", false); }

        //!
        //! Escape characters in a string to be used as tag key, tag value, or field key.
        //! @param [in] name The name to transform.
        //! @return The transformed string with escaped special characters.
        //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
        //!
        static UString ToKey(const UString& name) { return Escape(name, u",= \\", false); }

        //!
        //! Escape characters in a string to be used as field value of type string.
        //! Do not use for numerical field values. This function adds surrounding quotes.
        //! @param [in] name The name to transform.
        //! @return The transformed string with escaped special characters and surrounding quotes.
        //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
        //!
        static UString ToStringValue(const UString& name) { return Escape(name, u"\"\\", true); }

    private:
        const InfluxArgs& _args;
        std::intmax_t     _timestamp = 0;
        UString           _precision {};
        UString           _additional_tags {};
        UString           _builder {};

        // Helper for escape strings.
        static UString Escape(const UString& name, const UString& specials, bool add_quotes);
    };

    //!
    //! Safe pointer to an InfluxRequest.
    //! Typically used with MeesageQueue.
    //!
    using InfluxRequestPtr = std::shared_ptr<InfluxRequest>;
}
