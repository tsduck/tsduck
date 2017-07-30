#!/bin/bash
#
# Create the markdown text for a GitHub release.
# Steps:
# - Create and push the tag for the release.
# - Create the release on GitHub and upload binaries.
# - Run this script to generate the markdown text.
# - Edit the release on GitHub with this text.
#

SCRIPT=$(basename $0 .sh)
SCRIPTDIR=$(cd $(dirname $0); pwd)
info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Directories.
ROOTDIR=$(cd $SCRIPTDIR/..; pwd)
INSTALLDIR=$ROOTDIR/installers

# Get most recent git tag.
TAG=$(cd $ROOTDIR; git tag --sort=-taggerdate | head -1)
[[ -n "$TAG" ]] || error "no git tag found"

# Get content of web page for release.
HOST=https://github.com
URL=$HOST/tsduck/tsduck/releases/tag/$TAG
HTMLFILE=/tmp/tsduck-$TAG.html
LINKFILE=/tmp/tsduck-$TAG.links
wget -q -O "$HTMLFILE" "$URL" || error "error getting $URL"

# Extract all href in HTML file. We assume that there is at most 1 href by line.
sed -e '/href="/!d' -e 's|^.*href="||' -e 's|".*$||' -e "s|^/|$HOST/|" $HTMLFILE | \
    grep -e '\.exe' -e '\.deb' -e '\.rpm' >$LINKFILE 

# Get links for all expected targets.
getlink() {
    pattern="$1"
    link=$(grep "$pattern" $LINKFILE | tail -1)
    [[ -n "$link" ]] || error "no link found for $pattern"
    file=$(sed <<<$link -e 's|^.*/||')
    echo "[$file]($link)"
}
WIN32=$(getlink '/TSDuck-Win32-.*\.exe$')
WIN64=$(getlink '/TSDuck-Win64-.*\.exe$')
EL32=$(getlink '/tsduck-[0-9].*\.el[0-9]*\.i386\.rpm$')
EL64=$(getlink '/tsduck-[0-9].*\.el[0-9]*\.x86_64\.rpm$')
FC32=$(getlink '/tsduck-[0-9].*\.fc[0-9]*\.i386\.rpm$')
FC64=$(getlink '/tsduck-[0-9].*\.fc[0-9]*\.x86_64\.rpm$')
UB64=$(getlink '/tsduck_[0-9].*_amd64\.deb$')
EL32DEV=$(getlink '/tsduck-devel-[0-9].*\.el[0-9]*\.i386\.rpm$')
EL64DEV=$(getlink '/tsduck-devel-[0-9].*\.el[0-9]*\.x86_64\.rpm$')
FC32DEV=$(getlink '/tsduck-devel-[0-9].*\.fc[0-9]*\.i386\.rpm$')
FC64DEV=$(getlink '/tsduck-devel-[0-9].*\.fc[0-9]*\.x86_64\.rpm$')
UB64DEV=$(getlink '/tsduck-dev_[0-9].*_amd64\.deb$')

# Now create the markdown file.
MDFILE=$INSTALLDIR/github-release-$TAG.md
info "creating $MDFILE ..."
cat >$MDFILE <<EOF
Binaries for command-line tools and plugins:
* Windows 32 bits: $WIN32
* Windows 64 bits: $WIN64
* RedHat/CentOS 32 bits: $EL32
* RedHat/CentOS 64 bits: $EL64
* Fedora 32 bits: $FC32
* Fedora 64 bits: $FC64
* Ubuntu 64 bits: $UB64

Binaries for development environment:
* Windows: Included in installer (select option "Development")
* RedHat/CentOS 32 bits: $EL32DEV
* RedHat/CentOS 64 bits: $EL64DEV
* Fedora 32 bits: $FC32DEV
* Fedora 64 bits: $FC64DEV
* Ubuntu 64 bits: $UB64DEV

**Warning:** On Windows 64 bits, if you use DVB tuners which come with 32-bit drivers and DirectShow filters, you must use the 32 bits version of TSDuck. The 64-bit version of TSDuck cannot work with 32-bit drivers and filters.
EOF

# rm -f $HTMLFILE $LINKFILE
