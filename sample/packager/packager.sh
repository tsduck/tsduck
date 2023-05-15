#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2023, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  Demo script to encrypt an SPTS file with ECM insertion.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)

# Default command line parameter values.
# Mandatory options, without default:
INFILE=
OUTFILE=

# Conditional Access options (ECMG interface):
ECMG_ADDRESS=localhost
ECMG_PORT=4567
ECMG_CHANNEL_ID=1
ECMG_ECM_ID=1
SUPER_CAS_ID=0x12345678
ACCESS_CRITERIA=
CADESC_PRIV_DATA=

# Scrambling options:
ENCRYPTION=atis-idsa
ECM_BITRATE=15000

# Other options:
INIT_SECONDS=2
VERBOSE=false
VERBOSE_OPTS=

#-----------------------------------------------------------------------------
# Display help text
#-----------------------------------------------------------------------------

showhelp()
{
    cat >&2 <<EOF

Encrypt an SPTS file with ECM insertion.

Usage: $SCRIPT [options] input-file output-file

Parameters:

  The input-file must be a single-program transport stream file containing one
  clear service with proper PSI signalization. The output-file is created with
  encrypted video and audio and ECM insertion.

  -a hexa-string
  --access-criteria hexa-string
      Complete access criteria binary block. Use hexadecimal characters.
      Example: --access-criteria 0123456789

  --channel-id value
      ECM_channel_id parameter in the ECMG <=> SCS protocol. Default: $ECMG_CHANNEL_ID

  --ecm-bitrate value
      ECM bitrate in bits/second. Default: $ECM_BITRATE b/s.

  --ecm-id value
      ECM_id parameter in the ECMG <=> SCS protocol. Default: $ECMG_ECM_ID

  --ecmg-address ip-address
      IP address or host name of the ECMG. Default: $ECMG_ADDRESS

  --ecmg-port value
      TCP port of the ECMG. Default: $ECMG_PORT

  -e name
  --encryption name
      Content encryption algorithm. Must be one of atis-idsa, dvb-csa2,
      dvb-cissa, aes-cbc, aes-ctr. Default: $ENCRYPTION

  -h
  --help
      Display this help text.

  -i value
  --init-seconds value
      Number of seconds of stuffing to add at the beginning of the file. This
      time will be used by the STB to acquire the PMT, the ECM, extract the
      first CW and configure the descrambler. Default: $INIT_SECONDS

  -p hexa-string
  --private-data-ca-descriptor hexa-string
      Private data in the CA_descriptor for the ECM PID in the PMT. Use
      hexadecimal characters.

  -s value
  --super-cas-id value
      Super_CAS_id value for ECMG. Default: $SUPER_CAS_ID

  -v
  --verbose
      Display verbose information.

EOF
    exit 1
}

#-----------------------------------------------------------------------------
# Basic functions
#-----------------------------------------------------------------------------

# Name prefix for temporary files.
TMPFILE=/tmp/$$

# Use cleanup to exit with proper cleanup.
cleanup() { rm -rf "$TMPFILE"*; exit ${1:-0}; }
error()   { echo >&2 "$SCRIPT: $1"; cleanup 1; }
verbose() { $VERBOSE && echo "* $SCRIPT: $*"; }
usage()   { echo >&2 "invalid command, try \"$SCRIPT --help\""; cleanup 1; }

# Cleanup if interrupted.
trap cleanup SIGINT

#-----------------------------------------------------------------------------
# Decode command line arguments
#-----------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        -a|--access-criteria)
            [[ $# -gt 1 ]] || usage; shift
            ACCESS_CRITERIA="$1"
            ;;
        --channel-id)
            [[ $# -gt 1 ]] || usage; shift
            ECMG_CHANNEL_ID="$1"
            ;;
        --ecm-bitrate)
            [[ $# -gt 1 ]] || usage; shift
            ECM_BITRATE="$1"
            ;;
        --ecm-id)
            [[ $# -gt 1 ]] || usage; shift
            ECMG_ECM_ID="$1"
            ;;
        --ecmg-address)
            [[ $# -gt 1 ]] || usage; shift
            ECMG_ADDRESS="$1"
            ;;
        --ecmg-port)
            [[ $# -gt 1 ]] || usage; shift
            ECMG_PORT="$1"
            ;;
        -e|--encryption)
            [[ $# -gt 1 ]] || usage; shift
            ENCRYPTION="$1"
            ;;
        -h|--help)
            showhelp
            ;;
        -i|--init-seconds)
            [[ $# -gt 1 ]] || usage; shift
            INIT_SECONDS="$1"
            ;;
        -p|--private-data*)
            [[ $# -gt 1 ]] || usage; shift
            CADESC_PRIV_DATA="$1"
            ;;
        -s|--super-cas-id)
            [[ $# -gt 1 ]] || usage; shift
            SUPER_CAS_ID="$1"
            ;;
        -v|--verbose)
            VERBOSE=true
            VERBOSE_OPTS="--verbose"
            ;;
        -*)
            usage
            ;;
        *)
            if [[ -z "$INFILE" ]]; then
                INFILE="$1"
            elif [[ -z "$OUTFILE" ]]; then
                OUTFILE="$1"
            else
                usage
            fi
            ;;
    esac
    shift
done

# Input and output files are mandatory.
[[ -n "$INFILE" ]] || usage
[[ -n "$OUTFILE" ]] || usage

#-----------------------------------------------------------------------------
# Pre-processing
#-----------------------------------------------------------------------------

# Check existence of input file.
[[ -e "$INFILE" ]] || error "$INFILE not found"

# Analyze the file (normalized output format).
# Do not analyze more than 10000 packets, should be enough.
# We analyze the file to get the bitrate and find an unused PID for ECM's.
tsp $VERBOSE_OPTS \
    -I file "$INFILE" \
    -P until --packets 10000 \
    -P analyze --normalized -o "$TMPFILE.norm.txt" \
    -O drop \
    || cleanup 1

# Get TS bitrate in b/s, service id, PAT and PMT PID and bitrate.
TS_BITRATE=$(grep -m 1 '^ts:' "$TMPFILE.norm.txt" | sed -e 's/.*:bitrate=//' -e 's/:.*//')
PAT_BITRATE=$(grep -m 1 "^pid:pid=0:" "$TMPFILE.norm.txt" | sed -e 's/.*:bitrate=//' -e 's/:.*//')
SERVICE_ID=$(grep -m 1 '^service:' "$TMPFILE.norm.txt" | sed -e 's/.*:id=//' -e 's/:.*//')
PMT_PID=$(grep -m 1 '^service:' "$TMPFILE.norm.txt" | sed -e 's/.*:pmtpid=//' -e 's/:.*//')
PMT_BITRATE=$(grep -m 1 "^pid:pid=$PMT_PID:" "$TMPFILE.norm.txt" | sed -e 's/.*:bitrate=//' -e 's/:.*//')

# Error if some information is missing.
checkit() { [[ "$1" -gt 0 ]] || error "could not find $2, file may be invalid"; }
checkit "$TS_BITRATE"  "TS bitrate"
checkit "$PAT_BITRATE" "PAT bitrate"
checkit "$SERVICE_ID"  "service id"
checkit "$PMT_PID"     "PMT PID"
checkit "$PMT_BITRATE" "PMT bitrate"

# Find an unused PID for ECM, starting at 100
ECM_PID=100
while grep -q "^pid:pid=$ECM_PID:" "$TMPFILE.norm.txt"; do ECM_PID=$(( $ECM_PID + 1 )); done

# Number of stuffing packets to add at beginning of the file.
INIT_PACKETS=$(( ($INIT_SECONDS * $TS_BITRATE) / (188 * 8) ))

# Extract PAT and PMT of the service from the file.
# We need them to re-inject in the added leading stuffing.
tstables "$INFILE" --pid 0 --max 1 --bin "$TMPFILE.pat.bin" || cleanup 1
tstables "$INFILE" --pid $PMT_PID --max 1 --bin "$TMPFILE.pmt.bin" || cleanup 1

# Debug messages.
verbose "input file: $INFILE"
verbose "output file: $OUTFILE"
verbose "TS bitrate: $(printf %\'d $TS_BITRATE) b/s"
verbose "adding $(printf %\'d $INIT_PACKETS) initial suffing packets"
verbose "PMT PID: $(printf '0x%04X (%d)' $PMT_PID $PMT_PID)"
verbose "ECM PID: $(printf '0x%04X (%d)' $ECM_PID $ECM_PID)"
verbose "PAT bitrate: $(printf %\'d $PAT_BITRATE) b/s"
verbose "PMT bitrate: $(printf %\'d $PMT_BITRATE) b/s"
verbose "ECM bitrate: $(printf %\'d $ECM_BITRATE) b/s"
verbose "Access criteria: $ACCESS_CRITERIA"
verbose "CA_descriptor private data: $CADESC_PRIV_DATA"

#-----------------------------------------------------------------------------
# Now do the packaging in one single big tsp command
#-----------------------------------------------------------------------------

tsp $VERBOSE_OPTS --bitrate $TS_BITRATE --add-start-stuffing $INIT_PACKETS --add-input-stuffing 1/20 \
    -I file "$INFILE" \
    -P filter --pid 0 --pid $PMT_PID --negate --stuffing \
    -P inject "$TMPFILE.pat.bin" --pid 0 --bitrate $PAT_BITRATE --stuffing \
    -P inject "$TMPFILE.pmt.bin" --pid $PMT_PID --bitrate $PMT_BITRATE --stuffing \
    -P scrambler $SERVICE_ID \
        --synchronous --$ENCRYPTION \
        --access-criteria "$ACCESS_CRITERIA" \
        --super-cas-id $SUPER_CAS_ID \
        --ecmg $ECMG_ADDRESS:$ECMG_PORT \
        --channel-id $ECMG_CHANNEL_ID \
        --ecm-id $ECMG_ECM_ID \
        --pid-ecm $ECM_PID \
        --bitrate-ecm $ECM_BITRATE \
        --private-data "$CADESC_PRIV_DATA" \
    -P filter --after-packets $INIT_PACKETS --pid 0x1FFF --negate \
    -O file "$OUTFILE" \
    || cleanup 1

# Final success.
cleanup 0
