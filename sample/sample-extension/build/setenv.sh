# This script shall be source'd to make the extension visible to TSDuck directly after compilation.

SRC=$(cd $(dirname ${BASH_SOURCE[0]})/../src; pwd)

[[ ":$TSPLUGINS_PATH:" != *:$SRC:* ]] && export TSPLUGINS_PATH="$SRC:$TSPLUGINS_PATH"
[[ ":$PATH:" != *:$SRC:* ]] && export PATH="$SRC:$PATH"
