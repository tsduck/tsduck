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

namespace ts {
    //!
    //! Client request for an InfluxDB server.
    //! @ingroup libtscore net
    //! @see https://docs.influxdata.com/influxdb/v2/
    //!
    class TSCOREDLL InfluxRequest : public WebRequest
    {
        TS_NOBUILD_NOCOPY(InfluxRequest);
    public:
        //!
        //! Constructor.
        //! @param [in,out] report Where to report errors.
        //!
        InfluxRequest(Report& report);

        //!
        //! Destructor.
        //!
        virtual ~InfluxRequest() override;

        //!
        //! Send a write request to the InfluxDB server.
        //! @param [in] args The connection information to the InfluxDB server.
        //! @param [in] data The data to send, in "line protocol" format.
        //! @param [in] precision Precision of timestamps. Must be one of "s", "ms", "us", "ns".
        //! @return True on success, false on error.
        //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
        //! @see https://docs.influxdata.com/influxdb/v2/api/v2/#operation/PostWrite
        //!
        bool write(const InfluxArgs& args, const UString& data, const UString& precision);

        //!
        //! Escape characters in a string to be used as measurement.
        //! @param [in] name The name to transform.
        //! @return The transformed string with escaped special characters.
        //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
        //!
        static UString ToMeasurement(const UString& name) { return Escape(name, u", \\"); }

        //!
        //! Escape characters in a string to be used as tag key, tag value, or field key.
        //! @param [in] name The name to transform.
        //! @return The transformed string with escaped special characters.
        //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
        //!
        static UString ToKey(const UString& name) { return Escape(name, u",= \\"); }

        //!
        //! Escape characters in a string to be used as field value of type string.
        //! Do not use for numerical field values. This function adds surrounding quotes.
        //! @param [in] name The name to transform.
        //! @return The transformed string with escaped special characters and surrounding quotes.
        //! @see https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/
        //!
        static UString ToStringValue(const UString& name) { return Escape(name, u"\"\\"); }

    private:
        static UString Escape(const UString& name, const UString& specials);
    };
}
