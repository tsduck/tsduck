{
  description = "TSDuck - The MPEG Transport Stream Toolkit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    let
      # Development build flag: true if git tree is dirty (local dev), false for clean builds
      dev = self ? dirtyRev || self ? dirtyShortRev || !(self ? rev);

      # Build Python bindings for a given tsduck package.
      # Factored out so it can be called from both the overlay and per-system packages.
      mkPythonTsduck = { python3, lib, stdenv, tsduckPkg }:
        python3.pkgs.buildPythonPackage {
          pname = "tsduck";
          version = tsduckPkg.version;
          format = "other";

          src = lib.fileset.toSource {
            root = ../..;
            fileset = lib.fileset.unions [
              ../../src/libtsduck/python/tsduck.py
              ../../src/libtsduck/python/ts.py
            ];
          };

          propagatedBuildInputs = [ tsduckPkg ];
          dontBuild = true;

          installPhase = ''
            runHook preInstall
            mkdir -p $out/${python3.sitePackages}
            cp src/libtsduck/python/tsduck.py $out/${python3.sitePackages}/
            cp src/libtsduck/python/ts.py $out/${python3.sitePackages}/
            runHook postInstall
          '';

          postFixup =
            let
              libPath = "${tsduckPkg}/lib/libtsduck.${if stdenv.isDarwin then "dylib" else "so"}";
            in ''
              substituteInPlace $out/${python3.sitePackages}/tsduck.py \
                --replace-fail "return ctypes.util.find_library('tsduck')" 'return "${libPath}"'
            '';

          pythonImportsCheck = [ "tsduck" "ts" ];

          meta = with lib; {
            description = "Python bindings for TSDuck";
            homepage = "https://github.com/tsduck/tsduck";
            license = licenses.bsd2;
          };
        };

    in
    {
      overlays.default = final: prev: let
        # Hardware support packages – only available on x86_64-linux
        linuxX86Only = if final.stdenv.hostPlatform.system == "x86_64-linux" then {
          dtapi = final.callPackage ./dtapi.nix { };
          vatek = final.callPackage ./vatek.nix { };
        } else {
          dtapi = null;
          vatek = null;
        };
      in {
        tsduck = final.callPackage ./tsduck.nix {
          inherit dev;
          python3 = prev.python3;
          dtapi = linuxX86Only.dtapi;
          vatek = linuxX86Only.vatek;
          enableHides = true;
        };

        tsduck-min = final.callPackage ./tsduck.nix {
          inherit dev;
          python3 = prev.python3;
          dtapi = null;
          vatek = null;
          enableHides = false;
        };

        pythonPackagesExtensions = prev.pythonPackagesExtensions ++ [
          (python-final: python-prev: {
            tsduck = mkPythonTsduck {
              python3 = python-final.python;
              lib = final.lib;
              stdenv = final.stdenv;
              tsduckPkg = final.tsduck;
            };
            tsduck-min = mkPythonTsduck {
              python3 = python-final.python;
              lib = final.lib;
              stdenv = final.stdenv;
              tsduckPkg = final.tsduck-min;
            };
          })
        ];
      };
    }
    //
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ self.overlays.default ];
        };

        mkJava = tsduckPkg: pkgs.stdenv.mkDerivation {
          pname = "tsduck-java";
          version = tsduckPkg.version;

          dontUnpack = true;
          dontBuild = true;

          propagatedBuildInputs = [ pkgs.jdk tsduckPkg ];

          installPhase = ''
            runHook preInstall
            mkdir -p $out/share/java
            cp ${tsduckPkg}/share/tsduck/java/tsduck.jar $out/share/java/
            ln -s $out/share/java/tsduck.jar $out/share/java/tsduck-${tsduckPkg.version}.jar
            runHook postInstall
          '';

          meta = with pkgs.lib; {
            description = "Java bindings for TSDuck";
            homepage = "https://github.com/tsduck/tsduck";
            license = licenses.bsd2;
          };
        };
      in
      {
        packages = {
          default = pkgs.tsduck;

          # TSDuck C++ packages (provided by the overlay)
          tsduck     = pkgs.tsduck;
          tsduck-min = pkgs.tsduck-min;
        } // (if system == "x86_64-linux" then {
          # Third-party hardware support packages (x86_64-linux only, built separately)
          dtapi = pkgs.callPackage ./dtapi.nix { };
          vatek = pkgs.callPackage ./vatek.nix { };
        } else { }) // {
          # Python bindings (provided by the overlay via pythonPackagesExtensions)
          # These aliases preserve backward compatibility with:
          #   nix build ./pkg/nix#python-tsduck
          python-tsduck     = pkgs.python3Packages.tsduck;
          python-tsduck-min = pkgs.python3Packages.tsduck-min;

          # Java bindings
          java-tsduck     = mkJava pkgs.tsduck;
          java-tsduck-min = mkJava pkgs.tsduck-min;
        };

        devShells = {
          default = pkgs.mkShell {
            # Inherit all build dependencies from the full package (with Java)
            inputsFrom = [ pkgs.tsduck ];

            # Additional development tools
            packages = with pkgs; [
              gnumake
              pkg-config
              gdb
              # nix-prefetch-github is used by the ./pkg/nix/update-hw-sdks.sh script to
              # update sdk versions in dtapi.nix and vatek.nix
              nix-prefetch-github
            ] ++ lib.optional stdenv.isLinux [
                valgrind
            ];

            shellHook = ''
              echo "======================================"
              echo "TSDuck development environment"
              echo "======================================"
              echo ""
              echo "Available packages:"
              echo "  nix build ./pkg/nix#tsduck        - Build with all hardware support"
              echo "  nix build ./pkg/nix#tsduck-min    - Build TSDuck (minimal, no hardware)"
              echo "  nix build ./pkg/nix#python-tsduck - Build Python bindings"
              echo "  nix build ./pkg/nix#java-tsduck   - Build Java bindings"
              echo ""
              echo ""
              echo "Build commands:"
              echo "  make              - Build TSDuck"
              echo "  make test         - Run unit tests"
              echo ""
              echo ""
            '';
          };

          # Python development shell with TSDuck bindings
          python = pkgs.mkShell {
            packages = [
              (pkgs.python3.withPackages (ps: [
                ps.tsduck
              ]))
              pkgs.tsduck
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
            program = "${pkgs.tsduck}/bin/tsversion";
          };

          # Convenient shortcuts for common tools
          tsp = {
            type = "app";
            program = "${pkgs.tsduck}/bin/tsp";
          };

          tsanalyze = {
            type = "app";
            program = "${pkgs.tsduck}/bin/tsanalyze";
          };

          tsdump = {
            type = "app";
            program = "${pkgs.tsduck}/bin/tsdump";
          };

          # Python REPL with TSDuck bindings
          python = {
            type = "app";
            program = "${pkgs.python3.withPackages (ps: [
              ps.tsduck
            ])}/bin/python";
          };
        };
      }
    );
}
