#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  This script is used to locate java components on all Unix systems.
#
#-----------------------------------------------------------------------------

SCRIPT=$(basename ${BASH_SOURCE[0]})
error() { echo >&2 "$SCRIPT: $*"; exit 1; }
usage() { error "invalid option $*, try --help"; }

# Display help text
cmd_help() {
    cat >&2 <<EOF

Syntax: $SCRIPT [options]

  --bin      Directory for JDK executables
  --cflags   C++ pre-processor and compiler flags for JNI code
  --help     Display this help and exit
  --home     Jave home directory (typically for \$JAVA_HOME)
  --jar      JAR file manipulation command
  --java     Java execution command
  --javac    Java compiler command
  --jflags   Java command flags to allow JNI code
  --version  Get Java major version

EOF
    exit
}

# Check if a directory is a possible Java home for a JDK.
is_java_home() {
    [[ -n "$1" && -x "$1/bin/javac" && -e "$1/include/jni.h" ]]
}

# Check if a directory is a possible Java home for a JDK. Also print it.
print_java_home() {
    is_java_home "$1" && (cd "$1"; pwd)
}

# Locate Java home
cmd_home() {
    # Try explicit value
    print_java_home "$JAVA_HOME" && return
    # Need to search. Each OS has a distinct configuration.
    local system=$(uname -s)
    local jhome=
    local dir=
    local cmd=
    if [[ $system == Darwin ]]; then # macOS
        # Try fast method first, using a list of precise locations.
        for dir in \
            /Library/Java/JavaVirtualMachines/openjdk.jdk/Contents/Home \
            /opt/homebrew/opt/openjdk/libexec/openjdk.jdk/Contents/Home \
            /usr/local/opt/openjdk/libexec/openjdk.jdk/Contents/Home
        do
            print_java_home "$dir" && return
        done
        # Fallback to slower method, searching into directory trees.
        for dir in /Library/Java/JavaVirtualMachines/openjdk.jdk /Library/Java/JavaVirtualMachines; do
            cmd=$(find -L "$dir" -name javac -perm +444 2>/dev/null | tail -1)
            [[ -n "$cmd" ]] && print_java_home $(dirname "$cmd")/.. && return
        done
    elif [[ $system == FreeBSD || $system == DragonFly || $system == OpenBSD ]]; then
        # One or more version under /usr/local, use the last one.
        for dir in /usr/local/*jdk*; do
            is_java_home "$dir" && jhome="$dir"
        done
        [[ -n "$jhome" ]] && echo "$jhome"
    elif [[ $system == NetBSD ]]; then
        # One or more version under /usr/pkg/java, use the last one.
        for dir in /usr/pkg/java/*jdk*; do
            is_java_home "$dir" && jhome="$dir"
        done
        [[ -n "$jhome" ]] && echo "$jhome"
    elif [[ -f /etc/gentoo-release ]]; then
        # Gentoo Linux does not use symbolic links into jdk for java and javac
        dir=$(ls -d /etc/java-config*/current-system-vm 2>/dev/null | tail -1)
        [[ -n "$dir" ]] && dir=$(readlink -e -s "$dir")
        print_java_home "$dir"
    else
        # Linux, general case.
        cmd=$(which javac 2>/dev/null)
        [[ -z "$cmd" ]] && cmd=$(which java 2>/dev/null)
        [[ -n "$cmd" && -L "$cmd" ]] && cmd=$(readlink -e -s "$cmd")
        [[ -n "$cmd" ]] && print_java_home $(dirname "$cmd")/..
    fi
}

# Process individual commands
cmd_bin() {
    local home=$(cmd_home)
    [[ -n "$home" && -d "$home/bin" ]] && echo "$home/bin"
}

cmd_jar() {
    local home=$(cmd_home)
    [[ -n "$home" && -x "$home/bin/jar" ]] && echo "$home/bin/jar"
}

cmd_java() {
    local home=$(cmd_home)
    [[ -n "$home" && -x "$home/bin/java" ]] && echo "$home/bin/java"
}

cmd_version() {
    local java=$(cmd_java)
    [[ -n "$java" ]] && $java --version 2>/dev/null| sed -e '2,$d' -e 's/^[^0-9]*//' -e 's/[^0-9].*$//'
}

cmd_jflags() {
    # Starting with Java 24, access to native function through JNI is restricted.
    local version=$(cmd_version)
    [[ -n "$version" && "$version" -ge 24 ]] && echo "--enable-native-access=ALL-UNNAMED"
}

cmd_javac() {
    local home=$(cmd_home)
    [[ -n "$home" && -x "$home/bin/javac" ]] && echo "$home/bin/javac"
}

cmd_cflags() {
    local home=$(cmd_home)
    if [[ -n "$home" && -d "$home/include" ]]; then
        # JNI headers exist
        echo "-I$home/include$(dirname $(ls $home/include/*/jni_md.h 2>/dev/null) | sed -e 's/^/ -I/' | tr '\n' ' ')"
    else
        # Disable JNI code
        echo "-DTS_NO_JAVA=1"
    fi
}

if [ $# -eq 0 ]; then
    # No option, display everything.
    echo "home: $(cmd_home)"
    echo "bin: $(cmd_bin)"
    echo "version: $(cmd_version)"
    echo "java: $(cmd_java)"
    echo "javac: $(cmd_javac)"
    echo "jar: $(cmd_jar)"
    echo "jflags: $(cmd_jflags)"
    echo "cflags: $(cmd_cflags)"
else
    # Display options one by one.
    for arg in "$@"; do
        case "$arg" in
            --bin) cmd_bin ;;
            --cflags) cmd_cflags ;;
            --help) cmd_help ;;
            --home) cmd_home ;;
            --jar) cmd_jar ;;
            --java) cmd_java ;;
            --javac) cmd_javac ;;
            --jflags) cmd_jflags ;;
            --version) cmd_version ;;
            *) usage "$arg" ;;
        esac
        shift
    done
fi
