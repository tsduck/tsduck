#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script builds the TSDuck installers on a remote system. The remote
#  system can be a running physical machine or a local VM.
#
#  - In the case of a local VM, it is booted and shutdown if it does not
#    already run. Currently supported hypervisors for booting VM's:
#    - VMware (Player & Fusion)
#    - VirtualBox
#    - Parallels Desktop
#  - This script can be run from Linux, macOS or Windows (Cygwin, Git Bash).
#  - SSH access to remote systems and VM's shall be configured for direct
#    access without password. The corresponding user names and ports shall
#    be configured in the local ~/.ssh/config and the required public keys
#    shall be configured in the remote ~/.ssh/authorized_keys.
#  - Remotely built installers are copied bac into the local "installers"
#    subdirectory.
#  - An additional "backup directory" can be specified to store the built
#    installers (typically a network drive).
#
#  Sample user's script to build TSDuck binaries on four platforms, one
#  physical Raspberry Pi and three VMWare virtual machines:
#
#    $HOME/tsduck/pkg/build-remote.sh --host raspberry
#    $HOME/tsduck/pkg/build-remote.sh --host vmwindows --vmware $HOME/VM/Windows.vmwarevm/Windows.vmx --windows
#    $HOME/tsduck/pkg/build-remote.sh --host vmfedora --vmware $HOME/VM/Fedora.vmwarevm/Fedora.vmx
#    $HOME/tsduck/pkg/build-remote.sh --host vmubuntu --parallels Ubuntu
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename $BASH_SOURCE)
ROOTDIR=$(cd $(dirname $BASH_SOURCE)/..; pwd)

verbose() { $VERBOSE && echo >&2 "$*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }
usage() { echo >&2 "invalid command, try \"$SCRIPT --help\""; exit 1; }

# Default values for command line options.

VERBOSE=false
REMOTE_DIR=tsduck
REMOTE_WIN=false
LOCAL_BUILD=false
USER_NAME=
HOST_NAME=
HOST_IP=
VMX_FILE=
PRL_NAME=
VBOX_NAME=
QEMU_CMD=
SSH_TIMEOUT=5
SSH_PORT=
BOOT_TIMEOUT=500
SHUTDOWN_TIMEOUT=200
BACKUP_ROOT=
INSPREREQ=false
BUILDARGS=

#-----------------------------------------------------------------------------
# Display help text
#-----------------------------------------------------------------------------

showhelp()
{
    cat >&2 <<EOF

Build the TSDuck installers on a remote system and get them back.

Usage: $SCRIPT [options]

Options:

  --args "..."
      Build arguments. Directly passed to "make installer" (UNIX systems) or
      "build-installer.ps1" (Windows).

  --backup directory
      Specify a directory where all installers are copied. An intermediate
      subdirectory is created with the version name.

  --boot-timeout seconds
      Virtual machine boot timeout. Default: $BOOT_TIMEOUT seconds.

  -d path
  --directory path
      Relative path of the tsduck repository on the remote host, from the login
      directory of the remote user. Default: $REMOTE_DIR

  --help
      Display this help text.

  -h name
  --host name
      Build on this remote host. Mandatory for remote hosts and with --qemu.
      Optional for native virtual machines.

  --local
      No remote build, build locally. Useful for OS-independent script and
      support for --backup after build.

  --parallels name
      Use the Parallels Desktop virtual machine with the specified name. If the
      VM is not currently running, it is booted first and shut down after
      building the installers.

  -p number
  --port number
      Non-default TCP port for ssh and scp.

  --prerequisites
      Install prerequisites (scripts/install-prerequisites.sh) before building.
      Useful when a new version needs new dependencies which may not be installed
      on the remote system. Requires implicit sudo access on the remote system.

  --qemu 'command'
      Start an emulated virtual machine using Qemu. The specified command is run
      in the background, all along the build. This must be a Qemu command or a
      shell script running the required Qemu command. After the build, a shutdown
      command is sent to the emulated virtual machine using ssh.

  -t seconds
  --timeout seconds
      Connexion timeout for ssh and scp. Default: $SSH_TIMEOUT seconds.

  -u name
  --user name
      User name to use with ssh on the remote host, if different from local
      user name and not specified in .ssh/config.

  --vbox name
      Use the VirtualBox virtual machine with the specified name. If the VM
      is not currently running, it is booted first and shut down after building
      the installers.

  -v
  --verbose
      Verbose mode, add more messages.

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
        --args)
            [[ $# -gt 1 ]] || usage; shift
            BUILDARGS="$1"
            ;;
        --backup)
            [[ $# -gt 1 ]] || usage; shift
            BACKUP_ROOT="$1"
            ;;
        --boot-timeout)
            [[ $# -gt 1 ]] || usage; shift
            BOOT_TIMEOUT="$1"
            ;;
        -d|--directory)
            [[ $# -gt 1 ]] || usage; shift
            REMOTE_DIR="$1"
            ;;
        --help)
            showhelp
            ;;
        -h|--host)
            [[ $# -gt 1 ]] || usage; shift
            HOST_NAME="$1"
            ;;
        --local)
            LOCAL_BUILD=true
            ;;
        --parallels)
            [[ $# -gt 1 ]] || usage; shift
            PRL_NAME="$1"
            ;;
        -p|--port)
            [[ $# -gt 1 ]] || usage; shift
            SSH_PORT="$1"
            ;;
        --prereq*)
            INSPREREQ=true
            ;;
        --qemu)
            [[ $# -gt 1 ]] || usage; shift
            QEMU_CMD="$1"
            ;;
        -t|--timeout)
            [[ $# -gt 1 ]] || usage; shift
            SSH_TIMEOUT="$1"
            ;;
        -u|--user)
            [[ $# -gt 1 ]] || usage; shift
            USER_NAME="$1"
            ;;
        --vbox)
            [[ $# -gt 1 ]] || usage; shift
            VBOX_NAME="$1"
            ;;
        -v|--verbose)
            VERBOSE=true
            ;;
        --vmware)
            [[ $# -gt 1 ]] || usage; shift
            VMX_FILE="$1"
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
# Initialization stuff.
#-----------------------------------------------------------------------------

# Detect Windows.
case $(uname -s) in
    CYGWIN*) WINDOWS=true; SYSROOT=/cygdrive ;;
    MINGW*) WINDOWS=true; SYSROOT= ;;
    *) WINDOWS=false; SYSROOT= ;;
esac

# Use PuTTY for SSH access on remote builds from Windows.
if $WINDOWS && ! $LOCAL_BUILD; then
    # Make sure that PuTTY is installed.
    PLINK=$(which plink.exe)
    if [[ -z "$PLINK" ]]; then
        # Not found in the path, look into probable locations.
        PLINK=$(find $SYSROOT/c/Program*/PuTTY -iname plink.exe 2>/dev/null | head -1)
    fi
    [[ -z "$PLINK" ]] && error "PuTTY not installed, cannot SSH"
    # Enforce PuTTY for git ssh access.
    export GIT_SSH=$(cygpath -w "$PLINK")
    # Make sure that PuTTY Pageant is running for SSH authentication.
    [[ -z $(PowerShell -Command "(Get-Process -Name pageant).Name" 2>/dev/null) ]] && error "PuTTY Pageant not running, cannot SSH"
fi

# Home name is required with --qemu.
[[ -n "$QEMU_CMD" && -z "$HOST_NAME" ]] && error "--host is required with --qemu"

# Get the current date and time in normalized format.
curdate() {
    date +%Y%m%d-%H%M
}

# Extract the TSDuck version from an installer file name.
get-ts-version() {
    basename "$1" | sed -e 's/^.*\([0-9][0-9]*\.[0-9][0-9]*-[0-9][0-9]*\).*$/\1/'
}

# Name of backup directory for an installer. Create it if necessary.
# Also create a directory for the build logs.
get-backup-dir() {
    [[ -z "$BACKUP_ROOT" ]] && return
    local version=$(get-ts-version "$1")
    [[ -z "$version" ]] && return
    local dir="$BACKUP_ROOT/v$version"
    [[ -d "$dir/logs" ]] || mkdir -p "$dir/logs"
    [[ -d "$dir" ]] && echo "$dir"
}

# When on a Windows host, run a PowerShell command after cleaning up the Path.
# When this shell-script is run from Cygwin or MinGW, the Path contains Cygwin/MinGW-specific
# directories with commands which interfere with PowerShell (most notably: python).
powershell-cmd() {
    PATH=$(sed <<<":$PATH:" -e 's|:/usr/[^:]*||g' -e 's|:/bin[^:]*||g' -e 's|:/sbin[^:]*||g' -e 's|^:*||' -e 's|:*$||') \
        PowerShell -Command "$@"
}

#-----------------------------------------------------------------------------
# Remote build.
#-----------------------------------------------------------------------------

$LOCAL_BUILD && [[ -n "$HOST_NAME$VMX_FILE$PRL_NAME$VBOX_NAME$QEMU_CMD" ]] && error "cannot specify remote with --local"

# SSH and SCP options.
SSH_OPTS=()
[[ -n "$SSH_TIMEOUT" ]] && SSH_OPTS+=("-o" "ConnectTimeout=$SSH_TIMEOUT")
[[ -n "$SSH_PORT" ]] && SSH_OPTS+=("-p" "$SSH_PORT")

# Build SSH host specification.
ssh-name() {
    local name="$HOST_NAME"
    [[ -z "$name" ]] && name="$HOST_IP"
    [[ -z "$USER_NAME" ]] && echo "$name" || echo "$USER_NAME@$name"
}

# Host name as used in log file name.
if ! $LOCAL_BUILD; then
    NAME="$HOST_NAME"
elif $WINDOWS; then
    NAME=windows
else
    NAME=$(hostname)
    [[ -z "$NAME" ]] && NAME=linux
fi

# Process virtual machines startup.
VMX_SHUTDOWN=false
PRL_SHUTDOWN=false
VBOX_SHUTDOWN=false
SSH_SHUTDOWN=false
QEMU_PID=
if [[ -n "$VMX_FILE" ]]; then

    # Locate vmrun, the VMWare command line.
    # Try in PATH and then VMWare Fusion installation path on macOS.
    VMRUN=$(which vmrun 2>/dev/null)
    [[ -z "$VMRUN" ]] && VMRUN=$(ls "/Applications/VMware Fusion.app/Contents/Public/vmrun" 2>/dev/null)
    [[ -z "$VMRUN" ]] && error "vmrun not found, cannot manage VMWare VM's"
    [[ -z "$NAME" ]] && NAME=$(basename "$VMX_FILE" .vmx | tr A-Z a-z)

    # Try to get IP address of VM.
    HOST_IP=$("$VMRUN" getGuestIPAddress "$VMX_FILE")
    if [[ $? -ne 0 ]]; then

        # Cannot get VM IP address, try to boot the VM.
        echo Booting $(basename "$VMX_FILE")
        "$VMRUN" start "$VMX_FILE" nogui || error "cannot start VM in $VMX_FILE"

        # Wait until we can get the IP address of the VM.
        if [[ -z "$HOST_NAME" ]]; then
            maxdate=$(( $(date +%s) + $BOOT_TIMEOUT ))
            ok=1
            while [[ $(date +%s) -lt $maxdate ]]; do
                verbose "Trying to get IP address for $NAME"
                HOST_IP=$("$VMRUN" getGuestIPAddress "$VMX_FILE")
                ok=$?
                [[ $ok -eq 0 ]] && break
                sleep 5
            done
            [[ $ok -ne 0 ]] && error "Cannot get IP address of $NAME after $BOOT_TIMEOUT seconds"
            echo "IP address for $NAME is $IP, trying to ssh..."
        fi

        # Wait that the machine is accessible using ssh.
        # Don't wait once for boot timeout, sometimes it hangs.
        maxdate=$(( $(date +%s) + $BOOT_TIMEOUT ))
        ok=1
        while [[ $(date +%s) -lt $maxdate ]]; do
            verbose "Trying to ssh $(ssh-name)"
            ssh "${SSH_OPTS[@]}" $(ssh-name) cd &>/dev/null
            ok=$?
            [[ $ok -eq 0 ]] && break
            sleep 5
        done
        [[ $ok -ne 0 ]] && error "cannot contact VM $BOOT_TIMEOUT seconds after boot, aborting"
        echo "SSH ok for $(ssh-name)"

        # We need to shutdown the VM after building the installers.
        VMX_SHUTDOWN=true
    fi

    # Use the IP address if no host name is provided.
    [[ -z "$HOST_NAME" ]] && HOST_NAME="$HOST_IP"

elif [[ -n "$PRL_NAME" ]]; then

    # Locate prlctl, the Parallels Desktop command line.
    export PATH="/usr/local/bin:$PATH"
    [[ -z $(which prlctl 2>/dev/null) ]] && error "prlctl not found, cannot manage Parallels VM's"
    [[ -z $(which jq 2>/dev/null) ]] && error "jq not found, cannot manage Parallels JSON reports"

    # Name for log file.
    [[ -z "$HOST_NAME" ]] && NAME="$PRL_NAME" || NAME="$HOST_NAME"

    # Try to get IP address of VM.
    HOST_IP=$(prlctl list "$PRL_NAME" -f -j | jq -r '.[0].ip_configured')
    if [[ "$HOST_IP" != *.*.*.* ]]; then

        # Cannot get VM IP address, try to boot the VM.
        echo "Booting $PRL_NAME"
        prlctl start "$PRL_NAME" || error "cannot start VM $PRL_NAME"

        # Wait until we can get the IP address of the VM.
        if [[ -z "$HOST_NAME" ]]; then
            maxdate=$(( $(date +%s) + $BOOT_TIMEOUT ))
            while [[ $(date +%s) -lt $maxdate ]]; do
                HOST_IP=$(prlctl list "$PRL_NAME" -f -j | jq -r '.[0].ip_configured')
                [[ "$HOST_IP" == *.*.*.* ]] && break
                sleep 5
            done
            [[ "$HOST_IP" == *.*.*.* ]] || error "Cannot get IP address of $NAME after $BOOT_TIMEOUT seconds"
            echo "IP address for $NAME is $HOST_IP, trying to ssh..."
        fi

        # Wait that the machine is accessible using ssh.
        # Don't wait once for boot timeout, sometimes it hangs.
        maxdate=$(( $(date +%s) + $BOOT_TIMEOUT ))
        ok=1
        while [[ $(date +%s) -lt $maxdate ]]; do
            verbose "Trying to ssh $(ssh-name)"
            ssh "${SSH_OPTS[@]}" $(ssh-name) cd &>/dev/null
            ok=$?
            [[ $ok -eq 0 ]] && break
            sleep 5
        done
        [[ $ok -ne 0 ]] && error "cannot contact VM $BOOT_TIMEOUT seconds after boot, aborting"
        echo "SSH ok for $(ssh-name)"

        # We need to shutdown the VM after building the installers.
        PRL_SHUTDOWN=true
    fi

    # Use the IP address if no host name is provided.
    [[ -z "$HOST_NAME" ]] && HOST_NAME="$HOST_IP"

elif [[ -n "$VBOX_NAME" ]]; then

    # Locate VBoxManage, the VirtualBox command line.
    if ! $WINDOW || [[ $(which VBoxManage 2>/dev/null) ]]; then
        # Not on Windows or VBoxManage already in the path.
        VBOX=VBoxManage
    else
        # Search VBoxManage.
        VBOX=$(find $SYSROOT/c/Program*/Oracle/VirtualBox -iname VBoxManage.exe 2>/dev/null | head -1)
        [[ -z "$VBOX" ]] && error "VirtualBox not installed"
    fi

    # Function to get a property of a VM.
    vbox-get() {
        local host="$1"
        local name="$2"
        local value=$("$VBOX" guestproperty get "$host" "$name" 2>/dev/null | grep -i '^Value:' | sed -e 's/^Value: *//')
        [[ -z "$value" ]] && value=$("$VBOX" guestproperty enumerate "$host" | grep "^$name " | head -1 | sed -e "s|$name *= *'||" -e "s|'.*\$||")
        [[ -n "$value" ]] && echo "$value"
    }

    # Name for log file.
    [[ -z "$HOST_NAME" ]] && NAME="$VBOX_NAME" || NAME="$HOST_NAME"

    # Check if the VM is already running.
    if "$VBOX" list runningvms | grep -q "\"$VBOX_NAME\""; then
        true # already running
    else
        # Start the VM.
        echo "Booting $VBOX_NAME"
        "$VBOX" startvm "$VBOX_NAME" --type=headless

        # Wait until we can get the IP address of the VM.
        # We only get "host-only" IP addresses.
        HOST_SUBNET='192.168.56.'
        if [[ -z "$HOST_NAME" ]]; then
            maxdate=$(( $(date +%s) + $BOOT_TIMEOUT ))
            while [[ $(date +%s) -lt $maxdate ]]; do
                HOST_IP=$("$VBOX" guestproperty enumerate "$VBOX_NAME" "/VirtualBox/GuestInfo/Net/*/V4/IP" |
                         grep "$HOST_SUBNET" | head -1 | sed -e "s/^.* *= *'//" -e "s/'.*$//")
                [[ "$HOST_IP" == *.*.*.* ]] && break
                sleep 5
            done
            [[ "$HOST_IP" == *.*.*.* ]] || error "Cannot get IP address of $NAME after $BOOT_TIMEOUT seconds"
            echo "IP address for $NAME is $HOST_IP, trying to ssh..."
        fi

        # Wait that the machine is accessible using ssh.
        # Don't wait once for boot timeout, sometimes it hangs.
        maxdate=$(( $(date +%s) + $BOOT_TIMEOUT ))
        ok=1
        while [[ $(date +%s) -lt $maxdate ]]; do
            verbose "Trying to ssh $(ssh-name)"
            ssh "${SSH_OPTS[@]}" $(ssh-name) cd &>/dev/null
            ok=$?
            [[ $ok -eq 0 ]] && break
            sleep 5
        done
        [[ $ok -ne 0 ]] && error "cannot contact VM $BOOT_TIMEOUT seconds after boot, aborting"
        echo "SSH ok for $(ssh-name)"

        # We need to shutdown the VM after building the installers.
        VBOX_SHUTDOWN=true
    fi

elif [[ -n "$QEMU_CMD" ]]; then

    # Run a Qemu command in the background.
    echo "Running qemu in the background: $QEMU_CMD"
    $QEMU_CMD &
    QEMU_PID=$!

    # Wait that the machine is accessible using ssh.
    # Don't wait once for boot timeout, sometimes it hangs.
    maxdate=$(( $(date +%s) + $BOOT_TIMEOUT ))
    ok=1
    while [[ $(date +%s) -lt $maxdate ]]; do
        verbose "Trying to ssh $(ssh-name)"
        ssh "${SSH_OPTS[@]}" $(ssh-name) cd &>/dev/null
        ok=$?
        [[ $ok -eq 0 ]] && break
        sleep 5
    done
    if [[ $ok -ne 0 ]]; then
        kill -9 $QEMU_PID
        error "cannot contact qemu VM $BOOT_TIMEOUT seconds after boot, aborting"
    fi
    echo "SSH ok for $(ssh-name)"

    # We need to shutdown the VM after building the installers.
    SSH_SHUTDOWN=true

fi

# Check accessibility of remote host.
if ! $LOCAL_BUILD; then
    [[ -z "$HOST_NAME$HOST_IP" ]] && error "no remote host specified"
    verbose "Verifying ssh $(ssh-name)"
    ssh "${SSH_OPTS[@]}" $(ssh-name) cd &>/dev/null || error "$HOST_NAME not responding"
fi

# Build remote installers.
BACKUP_DIR=
BACKUP_MARK='##BACKUPDIR## '
LOGFILE="$ROOTDIR/pkg/installers/build-${NAME}-$(curdate).log"
(
    INSTALLERS=()

    if $LOCAL_BUILD; then
        # Local build, either Windows or Linux.
        (
            cd "$ROOTDIR"
            if $WINDOWS; then
                powershell-cmd ". scripts/cleanup.ps1 -NoPause"
                touch pkg/installers/timestamp.tmp
                $INSPREREQ && PREREQ=-Prerequisites || PREREQ=
                powershell-cmd ". pkg/nsis/build-installer.ps1 -GitPull $PREREQ -NoPause $BUILDARGS"
            else
                make clean
                git fetch origin && git checkout master && git pull origin master
                $INSPREREQ && scripts/install-prerequisites.sh
                touch pkg/installers/timestamp.tmp
                make installer $BUILDARGS
            fi
        )
        # Get all files from installers directory which are newer than the time stamp.
        for f in $(find "$ROOTDIR/pkg/installers" -maxdepth 1 -type f ! -iname '*.log' -newer "$ROOTDIR/pkg/installers/timestamp.tmp" -printf "%f "); do
            INSTALLERS+=("$f")
        done
        rm -f "$ROOTDIR/pkg/installers/timestamp.tmp"

    elif $REMOTE_WIN; then
        # Remote build on Windows. Important: assume PowerShell by default in SSH session.
        # Cleanup repository, rebuild from scratch.
        ssh "${SSH_OPTS[@]}" $(ssh-name) \
            ". '$REMOTE_DIR/scripts/cleanup.ps1' -NoPause"

        # Create a remote timestamp in installers subdirectory.
        # Newer files will be the installers we build.
        ssh "${SSH_OPTS[@]}" $(ssh-name) \
            "[void](New-Item -Type File '$REMOTE_DIR/pkg/installers/timestamp.tmp' -Force)"

        # Build installers after updating the repository.
        $INSPREREQ && PREREQ=-Prerequisites || PREREQ=
        ssh "${SSH_OPTS[@]}" $(ssh-name) \
            ". '$REMOTE_DIR/pkg/nsis/build-installer.ps1' -GitPull $PREREQ -NoPause $BUILDARGS"

        # Get all files from installers directory which are newer than the timestamp.
        files=$(ssh "${SSH_OPTS[@]}" $(ssh-name) \
            "Get-ChildItem '$REMOTE_DIR/pkg/installers' |
             Where-Object { \$_.LastWriteTime -gt (Get-Item '$REMOTE_DIR/pkg/installers/timestamp.tmp').LastWriteTime } |
             ForEach-Object { \$_.Name }" | tr '\r' ' ')

        # Copy all installers files.
        for f in $files; do
            echo "Fetching $f"
            INSTALLERS+=("$f")
            scp "${SSH_OPTS[@]}" $(ssh-name):"$REMOTE_DIR/pkg/installers/$f" "$ROOTDIR/pkg/installers/"
        done

        # Delete the temporary timestamp.
        ssh "${SSH_OPTS[@]}" $(ssh-name) \
            "[void](Remove-Item -Force '$REMOTE_DIR/pkg/installers/timestamp.tmp' -ErrorAction Ignore)"
    else
        # Remote build on Unix.
        # Create a remote timestamp. Newer files will be the installers we build.
        # Build installers from scratch after updating the repository.
        $INSPREREQ && PREREQ=scripts/install-prerequisites.sh || PREREQ=true
        verbose "Starting build"
        ssh "${SSH_OPTS[@]}" $(ssh-name) \
            cd "$REMOTE_DIR" '&&' \
            git fetch origin '&&' \
            git checkout master '&&' \
            git pull origin master '&&' \
            make clean ';' \
            $PREREQ ';' \
            touch "pkg/installers/timestamp.tmp" '&&' \
            make installer $BUILDARGS

        # Get all files from installers directory which are newer than the time stamp.
        verbose "Fetching installers"
        files=$(ssh "${SSH_OPTS[@]}" $(ssh-name) \
            find "$REMOTE_DIR/pkg/installers" -maxdepth 1 -type f -newer "$REMOTE_DIR/pkg/installers/timestamp.tmp" -printf "'%f '")

        # Copy all files from installers directory which are newer than the time stamp.
        for f in $files; do
            echo "Fetching $f"
            INSTALLERS+=("$f")
            scp "${SSH_OPTS[@]}" $(ssh-name):"$REMOTE_DIR/pkg/installers/$f" "$ROOTDIR/pkg/installers/"
        done

        # Delete the temporary timestamp.
        ssh "${SSH_OPTS[@]}" $(ssh-name) rm -f "$REMOTE_DIR/pkg/installers/timestamp.tmp"
    fi

    # Copy the installers in the backup directory if one is specified.
    for f in "${INSTALLERS[@]}"; do
        BACKUP_DIR=$(get-backup-dir "$f")
        if [[ -n "$BACKUP_DIR" ]]; then
            cp -v "$ROOTDIR/pkg/installers/$f" "$BACKUP_DIR/"
            # The build executes in a subshell and BACKUP_DIR won't be propagated in the parent shell.
            # Save it in the log file where the parent shell will retrieve it.
            echo "$BACKUP_MARK$BACKUP_DIR"
        fi
    done

) &>"$LOGFILE"

# Grab the backup directory path from the log file.
BACKUP_DIR=$(grep "^$BACKUP_MARK" "$LOGFILE" | head -1 | sed -e "s/^$BACKUP_MARK//")

# Copy the build log in the backup directory if one is specified.
[[ -n "$BACKUP_DIR" ]] && cp -v "$LOGFILE" "$BACKUP_DIR/logs/"

# Shutdown the VM if we booted it.
if $VMX_SHUTDOWN; then
    echo "Shutting down" $(basename "$VMX_FILE")
    "$VMRUN" stop "$VMX_FILE" soft
elif $PRL_SHUTDOWN; then
    echo "Shutting down $PRL_NAME"
    prlctl stop "$PRL_NAME"
elif $VBOX_SHUTDOWN; then
    # Proper shutdown if the guest additions are running, poweroff otherwise.
    # It is not possible to detect if the guest additions are _currently_ running.
    # Getting the guest OS version works if the guest additions had run once.
    # At least, we eliminate OS for which there is not guest addition (Alpine Linux, BSD).
    if [[ -z $(vbox-get "$VBOX_NAME" /VirtualBox/GuestInfo/OS/Version) ]]; then
        echo "Power off $VBOX_NAME"
        "$VBOX" controlvm "$VBOX_NAME" poweroff
    else
        echo "Shutting down $VBOX_NAME"
        "$VBOX" controlvm "$VBOX_NAME" shutdown --force
        # Wait until the VM disappear, power off after timeout.
        maxdate=$(( $(date +%s) + $SHUTDOWN_TIMEOUT ))
        while [[ $(date +%s) -lt $maxdate ]]; do
            if "$VBOX" list runningvms | grep -q "\"$VBOX_NAME\""; then
                sleep 5 # VM is still there
            else
                break
            fi
        done
        if "$VBOX" list runningvms | grep -q "\"$VBOX_NAME\""; then
            echo "Shutdown failed, power off"
            "$VBOX" controlvm "$VBOX_NAME" poweroff
        fi
    fi
elif $SSH_SHUTDOWN; then
    echo "Shutting down $(ssh-name) through SSH"
    ssh "${SSH_OPTS[@]}" $(ssh-name) sudo shutdown -h now
fi

# Wait for Qemu process completion.
if [[ -n "$QEMU_PID" ]]; then
    # There is a risk that the shutdown command is not accepted or hang. We do not want to wait forever.
    # Unfortunately, there is no timeout option on bash wait. So, we need an ancillary process to wait.
    verbose "Waiting for Qemu process termination"
    (sleep 30; kill -9 $QEMU_PID) &
    WAIT_PID=$!
    wait $QEMU_PID
    # Kill the ancillary process in case the shutdown completed before.
    kill -9 $WAIT_PID $QEMU_PID 2>/dev/null
fi

exit 0
