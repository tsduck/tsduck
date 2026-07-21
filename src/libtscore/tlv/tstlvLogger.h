//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2026, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  A logger class for TLV messages.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReporterBase.h"
#include "tstlvMessage.h"
#include "tsUString.h"

namespace ts::tlv {
    //!
    //! A logger class for TLV messages
    //! @ingroup libtscore tlv
    //!
    //! This class encapsulates a logging and debug facility for TLV messages.
    //! All messages are logged on a Report object under specific conditions.
    //! Each message, based on its tag, is logged with a specific severity.
    //! Depending on its maximum severity, the report will display or not
    //! each message.
    //!
    class TSCOREDLL Logger: public ReporterBase
    {
        TS_NOBUILD_NOCOPY(Logger);
    public:
        //!
        //! Constructor.
        //! @param [in] report Where to report errors. The @a report object must remain valid as long as this object
        //! exists or setReport() is used with another Report object. If @a report is null, log messages are discarded.
        //! @param [in] default_level Default logging level of messages.
        //!
        explicit Logger(Report* report, int default_level = Severity::Info);

        //!
        //! Constructor.
        //! @param [in] delegate Use the report of another ReporterBase. If @a delegate is null, log messages are discarded.
        //! @param [in] default_level Default logging level of messages.
        //!
        explicit Logger(ReporterBase* delegate, int default_level = Severity::Info);

        //!
        //! Destructor.
        //!
        virtual ~Logger() override;

        //!
        //! Set the default severity level for message logging.
        //! This level applies to messages without a specific log level.
        //! @param [in] level Default logging level of messages.
        //!
        void setDefaultSeverity(int level) { _default_level = level; }

        //!
        //! Get the default severity level for message logging.
        //! This level applies to messages without a specific log level.
        //! @return The default severity level.
        //!
        int defaultSeverity() const { return _default_level; }

        //!
        //! Set the severity level for one specific message tag.
        //! @param [in] tag Message tag.
        //! @param [in] level Logging level for messages using @a tag.
        //!
        void setSeverity(TAG tag, int level) { _levels[tag] = level; }

        //!
        //! Get the severity level for one specific message tag.
        //! @param [in] tag Message tag.
        //! @return The severity level.
        //!
        int severity(TAG tag) const;

        //!
        //! Reset all severities.
        //! @param [in] default_level Default logging level of messages.
        //!
        void resetSeverities(int default_level = Severity::Info);

        //!
        //! Report a TLV message.
        //! @param [in] msg The message to log.
        //! @param [in] comment Optional leading comment line (before the message).
        //!
        void log(const Message& msg, const UString& comment = UString()) const;

    private:
        int               _default_level;  // Default severity level.
        std::map<TAG,int> _levels {};      // Map of severity by message tag.
    };
}
