# Template setenv.sh for tsduck/src/MODULE/OBJDIR

curdir=$(dirname "${BASH_SOURCE[0]}")
curdir=$(cd "$curdir"; pwd)
obj=$(basename "$curdir")
srcdir=$(cd "$curdir/../.."; pwd)
libdir="$srcdir/libtsduck/$obj"
plgdir="$srcdir/tsplugins/$obj"
dtvdir="$srcdir/libtsduck/dtv"

[[ ":$PATH:" != *:$curdir:* ]] && export PATH="$curdir:$PATH"
[[ ":$LD_LIBRARY_PATH:" != *:$libdir:* ]] && export LD_LIBRARY_PATH="$libdir:$LD_LIBRARY_PATH"
[[ ":$TSPLUGINS_PATH:" != *:$dtvdir:* ]] && export TSPLUGINS_PATH="$dtvdir:$TSPLUGINS_PATH"
[[ ":$TSPLUGINS_PATH:" != *:$plgdir:* ]] && export TSPLUGINS_PATH="$plgdir:$TSPLUGINS_PATH"

true
