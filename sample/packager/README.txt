This directory contains a sample "VoD packager" script.

Overview
--------

In this demo, we assume that we manage a VoD system which uses transport stream
files which are encrypted using a classical Conditional Access System (admittedly
not a usual file format for a VoD system).

The script "packager.sh" prepares clear TS files for use with the VoD system.
The input file must be a single-program transport stream file containing one
clear service with proper PSI signalization. The encryption can be done using
ATIS-IDSA (AES) or DVB-CSA2. A DVB SimulCrypt-compliant ECMG is required to
generate the ECM's. For the purpose of the demo, the tool "tsemcg" will be used.
The output-file is created with encrypted video and audio and ECM insertion.

A given number of seconds of stuffing are added at the beginning of the output
file. During this period, PAT, PMT and ECM's are cycled but the rest is just
made of null packets. This period will be used by the STB to acquire the PMT,
the ECM, extract the first CW and configure the descrambler.

Implementation details
----------------------

Control words generation, encryption, ECM generation and insertion is directly
done by the tsp plugin "scrambler".

However, when the plugin "scrambler" starts, we are in the same situation as
in a "clear-to-scrambled transition", as defined by ETSI TS 103 197. During
the transition period, the PMT is updated, ECM's start and then, after the
"transition_delay_start" period, the content is scrambled. This transition
period is mandatory. Otherwise, STB's would not be able to locate, acquire
and process ECM's before the first encrypted packet.

When the tsp plugin "scrambler" is used on a live stream, it behaves like
any standard MUX. But for a VoD file, we want this transition period to occur
before the start of the file. So, in this specific script, we artificially
rebuild a fake transition period that we add before the first packet of the
original input file.

Now, let's explain in details how we do this. First, it is important to
remember that tsp processes one single transport stream. TS packets can be
modified, removed or replaced by null packets. But no tsp plugin can add
new packets in the transport stream. Here, we need to add packets in two
situations: the additional initial fake transition period and all ECM's
all along the file.

When a tsp plugin needs to inject new data, it "steals" stuffing, it replaces
null packets with the new packets to insert. But if there is no - or not
enough - null packets in the stream, the plugin cannot insert its data.

Fortunately, while a plugin cannot add new packets, the tsp framework can
artificially insert null packets at input level, before, between and after
input packets, as provided by the input plugin. See the tsp options
--add-input-stuffing, --add-start-stuffing and --add-stop-stuffing.

In the big tsp command which processes the file, we use the following two
options at tsp level (outside plugin options):

    --add-start-stuffing $INIT_PACKETS --add-input-stuffing 1/20

The option --add-start-stuffing adds some large number of null packets before
starting to read the input file. The amount of packets depends on the bitrate
of the input file and represents a playout duration of the clear-to-scrambled
transition. This is why the number of packets is dynamically computed.

The option --add-input-stuffing 1/20 inserts one null packet every 20 input
packets from the input file. These null packets, at regular intervals, will
be used to insert ECM packets. The ratio 1/20 is oversized, just to make sure
we can insert ECM packets when needed. The extra unused null packets will
be removed before writing the output file using the following plugin:

    -P filter --after-packets $INIT_PACKETS --pid 0x1FFF --negate

Now, we need to fill the initial transition period. The PAT and PMT must be
cycled here from the beginning so that the STB can discover the service and
the ECM PID. To do that, we first extract the PAT and PMT from the intput file
using two "tstables" commands, storing them into temporary files. These tables
will be re-injected in their original PID's at their original bitrates, but
starting at the beginning of the transition period, using the following
plugins:

    -P filter --pid 0 --pid $PMT_PID --negate --stuffing
    -P inject "$TMPFILE.pat.bin" --pid 0 --bitrate $PAT_BITRATE --stuffing
    -P inject "$TMPFILE.pmt.bin" --pid $PMT_PID --bitrate $PMT_BITRATE --stuffing

The last two plugins perform the table injection. But there is an additional
issue. While the PAT and PMT are correctly injected in the initial transition
period which contains null packets only, there will be a problem when the
injection reaches the start of the input file. There will be a PID clash with
the pre-existing PAT and PMT PID's. This is why we add the first plugin before
the two "inject" plugin. This first plugin removes the original PAT and PMT
PID's from the input stream and replaces them with stuffing (null packets).

As a last point, to build the final big tsp command, we need to know the
structure of the input file: TS bitrate (to compute $INIT_PACKETS), service
id, PMT PID, PAT and PMT bitrates. We do this by analysing the first 10000
packets of the file (no need to read a complete multi-gigabytes file). The
plugin "analyze" is used with option --normalized. The option produces an
output with a normalized format which is easy to parse using standard tools
such as grep and sed.
