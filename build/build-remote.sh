#!/bin/bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2018, Thierry Lelegard
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
#  This script builds the TSDuck installers on a remote system. The remote
#  system can be a running physical machine or a local VM. In the case of
#  a local VM, it is booted and shutdown if it does not already runs.
#  Currently supports VMWare only as hypervisor for local VM's.
#  Remotely built installers are copied into the local installers directory.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/..; pwd)
INSTDIR="$ROOTDIR/installers"

error() { echo >&2 "$SCRIPT: $*"; exit 1; }
usage() { echo >&2 "invalid command, try \"$SCRIPT --help\""; exit 1; }

# Default values for command line options.

REMOTE_DIR=tsduck
REMOTE_WIN=false
USER_NAME=$(id -un)
HOST_NAME=
VMX_FILE=


#-----------------------------------------------------------------------------
# Display help text
#-----------------------------------------------------------------------------

showhelp()
{
    cat >&2 <<EOF

Build the TSDuck installers on a remote system and get them back.

Usage: $SCRIPT [options]

Options:

  -d path
  --directory path
      Relative path of the tsduck repository on the remote host, from the login
      directory of the remote user. Default: $REMOTE_DIR

  --help
      Display this help text.

  -h name
  --host name
      Build on this remote host. Mandatory for remote hosts. Optional for VM's.

  -u name
  --user name
      User name to use with ssh on the remote host. Default: $USER_NAME

  --vmware filename
      Use the VMWare virtual machine from the specified .vmx file. If the VM
      is not currently running, it is booted first and shut down after building
      the installers.

  -w
  --windows
      Specify that the remote host is a Windows host. Default: Unix

EOF
    exit 1
}


#-----------------------------------------------------------------------------
# Decode command line arguments
#-----------------------------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--directory)
            [[ $# -gt 1 ]] || usage; shift
            REMOTE_DIR=$1
            ;;
        --help)
            showhelp
            ;;
        -h|--host)
            [[ $# -gt 1 ]] || usage; shift
            HOST_NAME=$1
            ;;
        -u|--user)
            [[ $# -gt 1 ]] || usage; shift
            USER_NAME=$1
            ;;
        --vmware)
            [[ $# -gt 1 ]] || usage; shift
            VMX_FILE=$1
            ;;
        -w|--windows)
            REMOTE_WIN=true
            ;;
        *)
            usage
            ;;
    esac
    shift
done


#-----------------------------------------------------------------------------
# Remote build.
#-----------------------------------------------------------------------------

curdate()  { date +%Y%m%d-%H%M; }
testping() { ping -o -q -t 2 "$1" &>/dev/null; }

[[ -z "$HOST_NAME" ]] && error "no remote host specified"
testping "$HOST_NAME" || error "$HOST_NAME not responding"

ssh "$USER_NAME@$HOST_NAME" make -C "$REMOTE_DIR" installer &>"$INSTDIR/build-${HOST_NAME}-$(curdate).log"

exit 0
