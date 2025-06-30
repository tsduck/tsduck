//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsUDPReceiverArgs.h"
#include "tsArgs.h"


//----------------------------------------------------------------------------
// Set application-specified parameters to receive unicast traffic.
//----------------------------------------------------------------------------

void ts::UDPReceiverArgs::setUnicast(const IPSocketAddress& local, bool reuse, size_t buffer_size)
{
    reuse_port = reuse;
    default_interface = false;
    no_link_local = false;
    use_first_source = false;
    mc_loopback = false;
    use_ssm = false;
    receive_bufsize = buffer_size;
    local_address = IPAddress(local);
    destination.clear();
    destination.setPort(local.port());
    source.clear();
}


//----------------------------------------------------------------------------
// Define command line options in an Args.
//----------------------------------------------------------------------------

void ts::UDPReceiverArgs::DefineArgs(Args& args, bool with_short_options, bool destination_is_parameter, bool multiple_receivers)
{
    // [[source@]address:]port can be a either a mandatory parameter or an option.
    const UChar* dest_name = DestName(destination_is_parameter);
    const UChar dest_short = destination_is_parameter || !with_short_options ? 0 : 'i';
    const size_t dest_min = destination_is_parameter ? 1 : 0;

    // [[source@]address:]port can be specified multiple times.
    const size_t max_count = multiple_receivers ? Args::UNLIMITED_COUNT : 1;
    const UChar* const dest_display = destination_is_parameter ? u"[address:]port parameters" : u"--ip-udp options";
    UString help;

    help = u"The [address:]port describes the destination of UDP packets to receive. "
           u"The 'port' part is mandatory and specifies the UDP port to listen on. "
           u"The 'address' part is optional. It specifies an IP multicast address to listen on. "
           u"It can be also a host name that translates to a multicast address. "
           u"An optional source address can be specified as 'source@address:port' in the case of SSM.";
    if (multiple_receivers) {
        help.format(u"\nSeveral %s can be specified to receive multiple UDP streams. "
                    u"If distinct receivers use the same port, this may work or not, depending on the operating system.",
                    dest_display);
    }
    args.option(dest_name, dest_short, Args::STRING, dest_min, max_count);
    args.help(dest_name, u"[[source@]address:]port", help);

    args.option(u"buffer-size", with_short_options ? 'b' : 0, Args::UNSIGNED);
    args.help(u"buffer-size", u"Specify the UDP socket receive buffer size in bytes (socket option).");

    args.option(u"default-interface");
    args.help(u"default-interface",
              u"Let the system find the appropriate local interface on which to listen. "
              u"By default, listen on all local interfaces.");

    args.option(u"disable-multicast-loop");
    args.help(u"disable-multicast-loop",
              u"Disable multicast loopback. By default, incoming multicast packets are looped back on local interfaces, "
              u"if an application sends packets to the same group from the same system. This option disables this.\n"
              u"Warning: On input sockets, this option is effective only on Windows systems. "
              u"On Unix systems (Linux, macOS, BSD), this option applies only to output sockets.");

    args.option(u"first-source", with_short_options ? 'f' : 0);
    args.help(u"first-source",
              u"Filter UDP packets based on the source address. Use the sender address of "
              u"the first received packet as only allowed source. This option is useful "
              u"when several sources send packets to the same destination address and port. "
              u"Accepting all packets could result in a corrupted stream and only one "
              u"sender shall be accepted. To allow a more precise selection of the sender, "
              u"use option --source. Options --first-source and --source are mutually "
              u"exclusive.");

    help = u"Specify the IP address of the local interface on which to listen. "
           u"It can be also a host name that translates to a local address. "
           u"By default, listen on all local interfaces.";
    if (multiple_receivers) {
        help.format(u"\nIf several %s are specified, several --local-address options can be specified, "
                    u"one for each receiver, in the same order. It there are less --local-address "
                    u"options than receivers, the last --local-address applies for all remaining receivers.",
                    dest_display);
    }
    args.option(u"local-address", with_short_options ? 'l' : 0, Args::IPADDR, 0, max_count);
    args.help(u"local-address", help);

    args.option(u"no-link-local");
    args.help(u"no-link-local",
              u"Do not join multicast groups from link-local addresses. "
              u"By default, join from all local interfaces.");

    args.option(u"no-reuse-port");
    args.help(u"no-reuse-port",
              u"Disable the reuse port socket option. Do not use unless completely necessary.");

    args.option(u"reuse-port", with_short_options ? 'r' : 0);
    args.help(u"reuse-port",
              u"Set the reuse port socket option. This is now enabled by default, the option "
              u"is present for legacy only.");

    args.option<cn::milliseconds>(u"receive-timeout");
    args.help(u"receive-timeout",
              u"Specify the UDP reception timeout in milliseconds. "
              u"This timeout applies to each receive operation, individually. "
              u"By default, receive operations wait for data, possibly forever.");

    help = u"Filter UDP packets based on the specified source address. This option is "
           u"useful when several sources send packets to the same destination address "
           u"and port. Accepting all packets could result in a corrupted stream and "
           u"only one sender shall be accepted. Options --first-source and --source "
           u"are mutually exclusive.";
    if (multiple_receivers) {
        help.format(u"\nIf several %s are specified, several --source options can be specified, "
                    u"one for each receiver, in the same order. If there are less --source "
                    u"options than receivers, the last --source applies for all remaining receivers.",
                    dest_display);
    }
    args.option(u"source", with_short_options ? 's' : 0, Args::IPSOCKADDR_OP, 0, max_count);
    args.help(u"source", help);

    args.option(u"ssm");
    args.help(u"ssm",
              u"Force the usage of Source-Specific Multicast (SSM) using the source which is specified by the option --source. "
              u"The --ssm option is implicit when the syntax 'source@address:port' is used.");
}


//----------------------------------------------------------------------------
// Load arguments from command line.
//----------------------------------------------------------------------------

bool ts::UDPReceiverArgs::loadArgs(Args& args,
                                   bool destination_is_parameter,
                                   size_t dest_index,
                                   cn::milliseconds default_receive_timeout,
                                   const IPAddress& default_local_address,
                                   const IPSocketAddress& default_source)
{
    bool ok = true;

    // General UDP options.
    reuse_port = !args.present(u"no-reuse-port");
    default_interface = args.present(u"default-interface");
    no_link_local = args.present(u"no-link-local");
    use_first_source = args.present(u"first-source");
    mc_loopback = !args.present(u"disable-multicast-loop");
    use_ssm = args.present(u"ssm");
    args.getIntValue(receive_bufsize, u"buffer-size", 0);
    args.getChronoValue(receive_timeout, u"receive-timeout", receive_timeout);

    local_address.clear();
    destination.clear();
    source.clear();

    const UChar* dest_name = DestName(destination_is_parameter);
    const size_t dest_count = args.count(dest_name);
    const size_t source_count = args.count(u"source");
    const size_t local_count = args.count(u"local-address");

    // There must be less --source and --local-address than destinations.
    if (source_count > dest_count) {
        args.error(u"too many --source options");
        ok = false;
    }
    if (local_count > dest_count) {
        args.error(u"too many --local-address options");
        ok = false;
    }

    // Either specify a local address or let the system decide, but not both.
    if (int(default_interface) + int(no_link_local) + int(local_count > 0) > 1) {
        args.error(u"--default-interface, --no-link-local, and --local-address are mutually exclusive");
        ok = false;
    }

    // Get all addresses.
    if (dest_index < dest_count) {

        // Start with destination address.
        UString dest_addr(args.value(dest_name, u"", dest_index));

        // Check the presence of the '@' indicating a source address.
        const size_t sep = dest_addr.find(u'@');
        if (sep != NPOS) {
            // Resolve source address.
            if (!source.resolve(dest_addr.substr(0, sep), args)) {
                ok = false;
            }
            // Force SSM.
            use_ssm = true;
            // Remove the source from the string.
            dest_addr.erase(0, sep + 1);
        }

        // Resolve destination address, after removing optional SSM source.
        if (!destination.resolve(dest_addr, args)) {
            ok = false;
        }

        // If a destination address is specified, it must be a multicast address.
        if (destination.hasAddress() && !destination.isMulticast()) {
            args.error(u"address %s is not multicast", destination);
            ok = false;
        }

        // In case of SSM, it should be in the SSM range, but let it a warning only.
        if (use_ssm) {
            if (!destination.hasAddress()) {
                args.error(u"multicast group address is missing with SSM");
                ok = false;
            }
            else if (!destination.isSSM()) {
                args.warning(u"address %s is not an SSM address", destination);
            }
            if (use_first_source) {
                args.error(u"SSM and --first-source are mutually exclusive");
                ok = false;
            }
        }

        // The destination port is mandatory
        if (!destination.hasPort()) {
            args.error(u"no UDP port specified in %s", dest_addr);
            ok = false;
        }

        // Get and resolve optional local address.
        args.getIPValue(local_address, u"local-address", default_local_address, dest_index);

        // If source is already set, it comes from source@destination SSM format and cannot be repeated through --source.
        if (source.hasAddress() && dest_index < source_count) {
            args.error(u"SSM source address specified twice");
            ok = false;
        }

        // If source is not set from source@destination SSM format, get --source.
        if (!source.hasAddress()) {
            args.getSocketValue(source, u"source", default_source, dest_index);
        }

        // If source is specified, the port is optional but the address is mandatory.
        if (dest_index < source_count && !source.hasAddress()) {
            args.error(u"missing IP address in --source %s", source);
            ok = false;
        }

        if (use_first_source && source.hasAddress()) {
            args.error(u"--first-source and --source are mutually exclusive");
            ok = false;
        }
        if (use_ssm && !source.hasAddress()) {
            args.error(u"missing source address with --ssm");
            ok = false;
        }
    }

    return ok;
}
