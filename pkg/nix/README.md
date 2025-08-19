# TSDuck Nix Packaging

This directory contains the Nix packaging for TSDuck and its optional hardware dependencies.

## Package Variants

### Main TSDuck Packages

- **`tsduck`** (default) — Full build with DekTec and VATek hardware support
- **`tsduck-min`** — Minimal build: no DekTec/VATek, but HiDes enabled
- **`python-tsduck`** / **`python-tsduck-min`** — Python bindings
- **`java-tsduck`** / **`java-tsduck-min`** — Java bindings

## Building

### From GitHub (flake URL):

```bash
# Default (full hardware support)
nix build github:tsduck/tsduck?dir=pkg/nix

# Minimal build
nix build github:tsduck/tsduck?dir=pkg/nix#tsduck-min

# Language bindings
nix build github:tsduck/tsduck?dir=pkg/nix#python-tsduck
nix build github:tsduck/tsduck?dir=pkg/nix#java-tsduck
```

### From local checkout:

```bash
cd tsduck/pkg/nix

nix build                  # default (tsduck, full hardware)
nix build .#tsduck-min     # minimal, no DekTec/VATek
nix build .#python-tsduck  # Python bindings (paired with tsduck)
nix build .#java-tsduck    # Java bindings  (paired with tsduck)
```

### Development shell:

```bash
# From GitHub
nix develop github:tsduck/tsduck?dir=pkg/nix

# From local checkout
cd tsduck/pkg/nix
nix develop
```

A Python-specific shell with TSDuck bindings pre-installed is also available:

```bash
nix develop .#python
```

## Updating Hardware SDK Versions

Use `update-hw-sdks.sh` to refresh the URLs and hashes in `dtapi.nix` and
`vatek.nix` to match the latest upstream releases:

```bash
# Update both
./update-hw-sdks.sh

# Update only one
./update-hw-sdks.sh --dtapi
./update-hw-sdks.sh --vatek
```

The script sources `scripts/dtapi-config.sh` for DekTec discovery and uses
the GitHub API for VATek.  It requires `nix`, `nix-prefetch-github`, `curl`,
and `jq`.

## Using TSDuck as a Flake Input

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    tsduck = {
      url = "github:tsduck/tsduck?dir=pkg/nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, tsduck }: {
    packages.x86_64-linux.default =
      tsduck.packages.x86_64-linux.tsduck;

    devShells.x86_64-linux.default =
      nixpkgs.legacyPackages.x86_64-linux.mkShell {
        buildInputs = [ tsduck.packages.x86_64-linux.tsduck ];
      };
  };
}
```
