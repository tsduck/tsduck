#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Joey Ekstrom
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Update the Nix fetchurl/fetchFromGitHub URLs and hashes in the hardware
#  SDK packages (pkg/nix/dtapi.nix, pkg/nix/vatek.nix) to match whatever
#  the upstream projects are currently shipping.
#
#-----------------------------------------------------------------------------

set -euo pipefail

SCRIPT=$(basename "$0")
ROOTDIR=$(cd "$(dirname "$0")/../.."; pwd)
SCRIPTSDIR="$ROOTDIR/scripts"
NIXDIR="$ROOTDIR/pkg/nix"

DTAPI_CONFIG="$SCRIPTSDIR/dtapi-config.sh"
VATEK_CONFIG="$SCRIPTSDIR/vatek-config.sh"

# Helper functions
info() { echo >&2 "$SCRIPT: $*"; }
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Wrapper to call dtapi-config.sh with the target platform for Nix (always Linux x86_64)
dtapi_config() {
    LOCAL_OS=linux LOCAL_ARCH=x86_64 "$DTAPI_CONFIG" "$@"
}

# Apply sed expressions to a file (in-place), or show the resulting diff
# in dry-run mode.
find_and_replace() {
    local file="$1"
    shift
    if $OPT_DRY_RUN; then
        echo "--- would apply to $file ---"
        sed "$@" "$file" | diff -u "$file" - || true
    else
        sed -i "$@" "$file"
    fi
}

# DTAPI
update_dtapi() {
    local nix_file="$NIXDIR/dtapi.nix"
    [[ -f "$nix_file" ]] || error "dtapi.nix not found at $nix_file"

    info "--- DTAPI ---"

    # Discover the current SDK tarball URL.
    local url=$(dtapi_config --url)
    [[ -n "$url" ]] || error "dtapi-config.sh --url returned empty"
    info "URL: $url"

    # Extract expected version from URL
    local expected_version=$(basename "$url" .tar.gz)
    expected_version=${expected_version#LinuxSDK_}
    expected_version=${expected_version#v}

    # Check if we already have the correct tarball cached
    local tarball=$(dtapi_config --tarball)
    local current_version=""
    if [[ -n "$tarball" ]]; then
        current_version=$(basename "$tarball" .tar.gz)
        current_version=${current_version#LinuxSDK_}
        current_version=${current_version#v}
    fi

    # Download if we don't have it or if the version doesn't match
    if [[ "$current_version" != "$expected_version" ]]; then
        info "downloading (cached: $current_version, expected: $expected_version) ..."
        dtapi_config --force --download
        tarball=$(dtapi_config --tarball)
    else
        info "using cached tarball (version matches)"
    fi

    [[ -n "$tarball" ]] || error "dtapi-config.sh --tarball returned empty (download may have failed)"
    info "tarball: $tarball"

    # Compute the Nix hash.
    info "computing hash ..."
    local hash_sri=$(nix hash file --type sha256 --sri "$tarball") \
        || error "nix hash file failed"
    info "hash: $hash_sri"

    # Get the version string from the tarball filename
    local version=$(basename "$tarball" .tar.gz)
    version=${version#LinuxSDK_}
    version=${version#v}
    [[ -n "$version" ]] || error "no version in filename"
    info "version: $version"

    # Patch dtapi.nix.
    info "updating $nix_file ..."
    find_and_replace "$nix_file" \
        -e "s|url = \"[^\"]*\";|url = \"$url\";|" \
        -e "s|hash = \"[^\"]*\";|hash = \"$hash_sri\";|" \
        -e "s|version = \"[^\"]*\";.*|version = \"$version\";|"

    info "DTAPI update complete"
}

# VATek
update_vatek() {
    local nix_file="$NIXDIR/vatek.nix"
    [[ -f "$nix_file" ]] || error "vatek.nix not found at $nix_file"

    info "--- VATek ---"
    info "discovering VATek release ..."
    local src_url=$("$VATEK_CONFIG" --src-url)
    [[ -n "$src_url" ]] || error "vatek-config.sh --src-url returned empty"
    info "src URL: $src_url"

    local tag=$(basename "$src_url")
    [[ -n "$tag" ]] || error "could not extract tag from src URL"
    info "tag: $tag"

    # Strip a leading 'v' for the bare version number used in vatek.nix.
    local version="${tag#v}"
    info "version: $version"

    info "prefetching from GitHub (this may take a moment) ..."
    local hash=$(nix-prefetch-url --unpack "$src_url" 2>/dev/null) \
        || error "nix-prefetch-url failed"

    # Convert to SRI hash format
    hash=$(nix hash convert --hash-algo sha256 --to sri "$hash") \
        || error "nix hash convert failed"
    info "hash: $hash"

    # Patch vatek.nix.
    info "updating $nix_file ..."
    find_and_replace "$nix_file" \
        -e "s|version = \"[^\"]*\";|version = \"$version\";|" \
        -e "s|hash = [^;]*;|hash = \"$hash\";|"

    info "VATek update complete"
}

# ---------------------------------------------------------------------------
# Option parsing
# ---------------------------------------------------------------------------
OPT_DTAPI=false
OPT_VATEK=false
OPT_DRY_RUN=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --dtapi)
            OPT_DTAPI=true  ;;
        --vatek)
            OPT_VATEK=true  ;;
        --dry-run)
            OPT_DRY_RUN=true ;;
        --help|-h)
            echo "Usage: $SCRIPT [--dtapi] [--vatek] [--dry-run]"
            echo "  --dtapi   Update pkg/nix/dtapi.nix"
            echo "  --vatek   Update pkg/nix/vatek.nix"
            echo "  --dry-run Print changes without writing files"
            echo "  (no flags) Update both"
            exit 0
            ;;
        *)
            error "invalid option $1 (use --dtapi, --vatek, --dry-run, --help)"
            ;;
    esac
    shift
done

# If neither was specified, do both.
if ! $OPT_DTAPI && ! $OPT_VATEK; then
    OPT_DTAPI=true
    OPT_VATEK=true
fi

# ---------------------------------------------------------------------------
$OPT_DTAPI && update_dtapi
$OPT_VATEK && update_vatek
