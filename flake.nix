{
  description = "TSDuck - The MPEG Transport Stream Toolkit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        # Get version from git describe
        # When the flake is locked, self.rev will be available
        # For local development, we compute it from git
        version =
          if self ? rev then
            self.shortRev
          else
            let
              gitOutput = builtins.readFile (pkgs.runCommandLocal "get-git-version" { } ''
                cd ${self}
                ${pkgs.git}/bin/git describe --tags --always --dirty 2>/dev/null > $out || echo "dev" > $out
              '');
            in
              pkgs.lib.removeSuffix "\n" gitOutput;
      in
      {
        packages = {
          default = self.packages.${system}.tsduck;

          # Full build with Java bindings
          tsduck = pkgs.callPackage ./pkg/nix/package.nix {
            inherit version;
          };

          # Python bindings for TSDuck (from passthru, uses minimal build)
          python-tsduck = self.packages.${system}.tsduck.python;

          # Java bindings for TSDuck (from passthru, only available with Java-enabled build)
          java-tsduck = self.packages.${system}.tsduck.java;
        };

        devShells = {
          default = pkgs.mkShell {
            # Inherit all build dependencies from the full package (with Java)
            inputsFrom = [ self.packages.${system}.tsduck ];

            # Additional development tools
            packages = with pkgs; [
              # Build essentials
              gnumake
              pkg-config

              # Development utilities
              gdb
              valgrind

              # Optional: tools for working with transport streams
              # (these are provided by the tsduck package itself once built)
            ];

            shellHook = ''
              echo "======================================"
              echo "TSDuck development environment"
              echo "======================================"
              echo ""
              echo "Available packages:"
              echo "  nix build         - Build TSDuck (with Java)"
              echo "  nix build .#python-tsduck  - Build Python bindings"
              echo "  nix build .#java-tsduck  - Build Java bindings"
              echo ""
              echo "Build commands:"
              echo "  make              - Build TSDuck"
              echo "  make test         - Run unit tests"
              echo "  make install      - Install (respects SYSROOT)"
              echo ""
              echo "See CONFIG.txt for build configuration options"
              echo "See Makefile for all available targets"
              echo ""
            '';
          };

          # Python development shell with TSDuck bindings
          python = pkgs.mkShell {
            packages = [
              (pkgs.python3.withPackages (ps: [
                self.packages.${system}.python-tsduck
              ]))
              self.packages.${system}.tsduck
            ];

            shellHook = ''
              echo "======================================"
              echo "TSDuck Python environment"
              echo "======================================"
              echo ""
              echo "Python with TSDuck bindings available"
              echo ""
              echo "Try:"
              echo "  python -c 'import tsduck; print(tsduck.__doc__)'"
              echo "  python -c 'import ts; print(\"Backward compatibility alias works!\")"
              echo ""
              echo "TSDuck tools also available:"
              echo "  tsversion"
              echo "  tsp --help"
              echo ""
            '';
          };
        };

        apps = {
          # Default app shows version
          default = {
            type = "app";
            program = "${self.packages.${system}.default}/bin/tsversion";
          };

          # Convenient shortcuts for common tools
          tsp = {
            type = "app";
            program = "${self.packages.${system}.default}/bin/tsp";
          };

          tsanalyze = {
            type = "app";
            program = "${self.packages.${system}.default}/bin/tsanalyze";
          };

          tsdump = {
            type = "app";
            program = "${self.packages.${system}.default}/bin/tsdump";
          };

          # Python REPL with TSDuck bindings
          python = {
            type = "app";
            program = "${pkgs.python3.withPackages (ps: [
              self.packages.${system}.python-tsduck
            ])}/bin/python";
          };
        };
      }
    );
}
