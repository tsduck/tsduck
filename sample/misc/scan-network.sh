#!/usr/bin/env bash
# Scan a few known broadcast networks.
# See function showhelp() below.

SCRIPT=$(basename "$BASH_SOURCE")
SCRIPTDIR=$(cd $(dirname "$BASH_SOURCE"); pwd)

#-----------------------------------------------------------------------------
# Known networks
#-----------------------------------------------------------------------------

# Astra 19.2 E (SES Astra)
NETW_NAMES[0]=astra
NETW_DVBNIT[0]="--delivery-system DVB-S --modulation QPSK --frequency 11,597,000,000 --polarity vertical --symbol-rate 22,000,000 --fec 5/6"

# Hot Bird 13.0 E (Eutelsat)
NETW_NAMES[1]=hotbird
NETW_DVBNIT[1]="--delivery-system DVB-S --modulation QPSK --frequency 12,731,000,000 --polarity horizontal --symbol-rate 29,900,000 --fec-inner 5/6"

#-----------------------------------------------------------------------------
# Display help text
#-----------------------------------------------------------------------------

showhelp()
{
    cat >&2 <<EOF

Scan a few known broadcast networks. Create the following output files:
- List of tuning options for each TS: <netname>-dvb-options.txt
- For each TS:
  . Analysis file: <netname>-ts<tsid>-<date>.txt
  . Optional capture file: <netname>-ts<tsid>-<date>.ts

Usage: $SCRIPT [options] netname

Parameter:

  netname
      Network name, one of: ${NETW_NAMES[*]}.

Options:

  -c
  --capture
      Get a capture file of each transport stream. Can produce very large
      files. By default, only analyze each transport stream.

  -d seconds
  --duration seconds
      Specify analysis duration in seconds on each transport stream.
      Default: $DURATION seconds.

  -h
  --help
      Display this help text.

  -o dir
  --output dir
      Specify output directory. Default: <netname>-<date>.

  -r
  --reuse
      Reuse existing files to speed up the scan when retrying after a failure.

EOF
    exit 1
}

#-----------------------------------------------------------------------------
# Default command line parameter values.
#-----------------------------------------------------------------------------

NETNAME=
NITTUNE=
CAPTURE=false
REUSE=false
DURATION=30
OUTDIR=
DATE=$(date +%Y%m%d)

#-----------------------------------------------------------------------------
# Basic functions
#-----------------------------------------------------------------------------

error() { echo >&2 "$SCRIPT: $*"; exit 1; }
info()  { echo >&2 "$SCRIPT: $*"; }
usage() { echo >&2 "invalid command, try \"$SCRIPT --help\""; exit 1; }

# Cygwin portability oddities.
if [[ $(uname -o) == Cygwin ]]; then
    path() { cygpath --windows "$1"; }
else
    path() { echo "$1"; }
fi

#-----------------------------------------------------------------------------
# Decode command line arguments
#-----------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        -c|--capture)
            CAPTURE=true
            ;;
        -d|--duration)
            [[ $# -gt 1 ]] || usage; shift
            DURATION="$1"
            ;;
        -h|--help)
            showhelp
            ;;
        -o|--output)
            [[ $# -gt 1 ]] || usage; shift
            OUTDIR="$1"
            ;;
        -r|--reuse)
            REUSE=true
            ;;
        -*)
            usage
            ;;
        *)
            [[ -z "$NETNAME" ]] || usage
            NETNAME="$1"
            ;;
    esac
    shift
done

# Check presence and validity of network name.
[[ -n "$NETNAME" ]] || usage
for ((i = 0; $i < ${#NETW_NAMES[*]}; i++)); do
    if [[ "$NETNAME" == "${NETW_NAMES[$i]}" ]]; then
        NITTUNE="${NETW_DVBNIT[$i]}"
        break
    fi
done
[[ -n "$NITTUNE" ]] || error "unknown network $NETNAME, use one of ${NETW_NAMES[*]}"

# Default output directory.
[[ -z "$OUTDIR" ]] && OUTDIR="$NETNAME-$DATE"
mkdir -p "$OUTDIR" || error "error creating $OUTDIR"

# Additional tuning options.
DVBOPTS="--receive-timeout 5000"

#-----------------------------------------------------------------------------
# Scan the NIT on reference transport stream.
#-----------------------------------------------------------------------------

NITFILE="$OUTDIR/$NETNAME-dvb-options.txt"
NITFILE_PATH=$(path "$NITFILE")

if [[ -s "$NITFILE" ]] && $REUSE; then
    info "reusing existing $NITFILE"
else
    info scanning the NIT
    tsp -v \
        -I dvb $DVBOPTS $NITTUNE \
        -P nitscan --output-file "$NITFILE_PATH" --dvb-options --comment --terminate \
        -O drop
fi

#-----------------------------------------------------------------------------
# Scan each transport stream.
#-----------------------------------------------------------------------------

TSID=
while read line; do
    line=$(sed <<<$line -e 's/\r$//')
    if grep <<<$line -q "# TS id:"; then

        # Got a comment line with a TS id.
        TSID=$(sed <<<$line -e 's/# TS id: *//' -e 's/ .*//')

    elif [[ "$line" = -* ]]; then

        # Got a line with tuning options.
        ANFILE="$OUTDIR/$NETNAME-ts$TSID-$DATE.txt"
        TSFILE="$OUTDIR/$NETNAME-ts$TSID-$DATE.ts"

        ANFILE_PATH=$(path "$ANFILE")
        TSFILE_PATH=$(path "$TSFILE")

        NEED_ANFILE=true
        NEED_TSFILE=$CAPTURE
        if $REUSE; then
            [[ -s "$ANFILE" ]] && NEED_ANFILE=false
            [[ -s "$TSFILE" ]] && NEED_TSFILE=false
        fi

        if ! $NEED_ANFILE && ! $NEED_TSFILE; then
            info "reusing existing files for TS id $TSID"
        else
            info "analyzing TS id $TSID during $DURATION seconds"
            if $CAPTURE; then
                tsp -v \
                    -I dvb $DVBOPTS $line \
                    -P until --seconds $DURATION \
                    -P analyze --output-file "$ANFILE_PATH" \
                    -O file "$TSFILE_PATH"
            else
                tsp -v \
                    -I dvb $DVBOPTS $line \
                    -P until --seconds $DURATION \
                    -P analyze --output-file "$ANFILE_PATH" \
                    -O drop
            fi
        fi
    fi
done <$NITFILE
