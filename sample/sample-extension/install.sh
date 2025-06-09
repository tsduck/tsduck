# Build and install TSDuck and the "foo" extension in the bin area.
# Use it with a bash "source" command.

FOO_DIR=$(cd $(dirname "$BASH_SOURCE"); pwd)
FOO_TSDUCK=$(cd "$FOO_DIR/../.."; pwd)
FOO_SYSROOT="$FOO_TSDUCK/bin/sysroot"

# Prepend an element to a search path. Syntax: pathmunge varname dirname
foo_pathmunge () {
    local varname="$1"
    local dirname="$2"
    # Remove all previous occurrences
    local path=":${!varname}:"
    path="${path//:$dirname:/:}"
    path="$dirname$path"
    # Cleanup path
    while [[ "$path" == :* ]]; do path="${path/#:/}"; done
    while [[ "$path" == *: ]]; do path="${path/%:/}"; done
    path="${path//::/:}"
    export $varname="$path"
}

# Build and install TSDuck in the temporary system root.
rm -rf "$FOO_SYSROOT"
make -C "$FOO_TSDUCK" -j10
make -C "$FOO_TSDUCK" install SYSPREFIX="$FOO_SYSROOT"

# Prepend the temporary system root.
# Duplicate LD_LIBRARY_PATH in LD_LIBRARY_PATH2 for macOS
# (LD_LIBRARY_PATH is not passed to scripts for security reasons).
foo_pathmunge PATH "$FOO_SYSROOT/bin"
foo_pathmunge LD_LIBRARY_PATH "$FOO_SYSROOT/lib"
export LD_LIBRARY_PATH2="$LD_LIBRARY_PATH"

# Build and install the "foo" extension in the temporary system root.
make -C "$FOO_DIR" -j10
make -C "$FOO_DIR" install

echo tsp is $(which tsp)
