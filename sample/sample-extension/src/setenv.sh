# This script shall be source'd to make the extension visible to TSDuck directly in this directory.
DIR=$(cd $(dirname ${BASH_SOURCE[0]}); pwd)
[[ ":$TSPLUGINS_PATH:" != *:$DIR:* ]] && export TSPLUGINS_PATH="$DIR:$TSPLUGINS_PATH"
[[ ":$PATH:" != *:$DIR:* ]] && export PATH="$DIR:$PATH"
