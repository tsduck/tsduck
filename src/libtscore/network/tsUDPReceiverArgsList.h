//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Command line arguments for the class UDPReceiver.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsUDPReceiverArgs.h"

namespace ts {
    //!
    //! Command line arguments for the class UDPReceiver.
    //! On the command line, depending on the application, zero, one, or more, receivers can be specified.
    //! @ingroup libtscore net
    //!
    class TSCOREDLL UDPReceiverArgsList: public std::vector<UDPReceiverArgs>
    {
    public:
        //!
        //! Add command line option definitions in an Args.
        //! @param [in,out] args Command line arguments to update.
        //! @param [in] with_short_options When true, define one-letter short options.
        //! @param [in] destination_is_parameter When true, the destination [address:]port is defined as a mandatory parameter.
        //! When false, it is defined as option --ip--udp (optional, can be omitted)
        //! @param [in] multiple_receivers When true, multiple destination [address:]port are allowed.
        //!
        void defineArgs(Args& args, bool with_short_options, bool destination_is_parameter, bool multiple_receivers)
        {
            _dest_is_parameter = destination_is_parameter;
            UDPReceiverArgs::DefineArgs(args, with_short_options, destination_is_parameter, multiple_receivers);
        }

        //!
        //! Load arguments from command line.
        //! Must be called after defineArgs().
        //! Args error indicator is set in case of incorrect arguments.
        //!
        //! Upon return, the number of elements in this instance depends on defineArgs() parameters:
        //!
        //! | @a destination_is_parameter | @a multiple_receivers | Number of elements
        //! | --------------------------- | --------------------- | ---------------------------
        //! | true                        | true                  | 1 or more
        //! | true                        | false                 | 1
        //! | false                       | true                  | 0 or more
        //! | false                       | false                 | 0 or 1
        //!
        //! @param [in,out] args Command line arguments.
        //! @param [in] default_receive_timeout Default receive timeout in milliseconds. No timeout if zero or negative.
        //! @return True on success, false on error in argument line.
        //!
        bool loadArgs(Args& args, cn::milliseconds default_receive_timeout = cn::milliseconds(-1));

    private:
        bool _dest_is_parameter = true;
    };
}
