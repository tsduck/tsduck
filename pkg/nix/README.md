# TSDuck Nix Packaging

This directory contains the Nix packaging for TSDuck and its optional hardware dependencies.

## Package Variants

### Main TSDuck Packages

- **`tsduck`** (default) — Full build with DekTec and VATek hardware support
- **`tsduck-min`** — Minimal build: no DekTec/VATek, but HiDes enabled
- **`python-tsduck`** / **`python-tsduck-min`** — Python bindings
- **`java-tsduck`** / **`java-tsduck-min`** — Java bindings

### Overlay

The flake exports `overlays.default` which adds the following to nixpkgs:

- **`pkgs.tsduck`** — Full TSDuck C++ package
- **`pkgs.tsduck-min`** — Minimal TSDuck (no hardware SDKs)
- **`pkgs.python3Packages.tsduck`** — Python bindings (full)
- **`pkgs.python3Packages.tsduck-min`** — Python bindings (minimal)

The Python packages are injected via `pythonPackagesExtensions`, making them
available under all Python interpreters (e.g. `python311Packages`,
`python312Packages`, `python313Packages`) and work with `python3.withPackages`.

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

### Recommended: Using the overlay

The overlay is the recommended way to consume TSDuck from another flake.
It integrates TSDuck into your nixpkgs instance so packages are available
as `pkgs.tsduck`, `pkgs.python3Packages.tsduck`, etc.

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    tsduck = {
      url = "github:tsduck/tsduck?dir=pkg/nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, tsduck, ... }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ tsduck.overlays.default ];
      };
    in {
      # Use TSDuck C++ tools
      packages.${system}.default = pkgs.tsduck;

      # Python with TSDuck bindings
      packages.${system}.python-env = pkgs.python3.withPackages (ps: [
        ps.tsduck
      ]);

      # Development shell
      devShells.${system}.default = pkgs.mkShell {
        packages = [
          pkgs.tsduck
          (pkgs.python3.withPackages (ps: [ ps.tsduck ]))
        ];
      };
    };
}
```

### Alternative: Direct package references

You can also reference packages directly without using the overlay:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
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
