{
  description = "TSDuck - The MPEG Transport Stream Toolkit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        # Development build flag: true if git tree is dirty (local dev), false for clean builds
        dev = self ? dirtyRev || self ? dirtyShortRev || !(self ? rev);

        # Only available on x86_64-linux
        linuxX86Only = if system == "x86_64-linux" then {
          dtapi = pkgs.callPackage ./dtapi.nix { };
          vatek = pkgs.callPackage ./vatek.nix { };
        } else {
          dtapi = null;
          vatek = null;
        };

        # Python bindings parameterised on which tsduck package they wrap.
        mkPythonTsduck = tsduckPkg: pkgs.python3.pkgs.buildPythonPackage {
          pname = "tsduck";
          version = tsduckPkg.version;
          format = "other";

          src = pkgs.lib.fileset.toSource {
            root = ../..;
            fileset = pkgs.lib.fileset.unions [
              ../../src/libtsduck/python/tsduck.py
              ../../src/libtsduck/python/ts.py
            ];
          };

          propagatedBuildInputs = [ tsduckPkg ];
          dontBuild = true;

          installPhase = ''
            runHook preInstall
            mkdir -p $out/${pkgs.python3.sitePackages}
            cp src/libtsduck/python/tsduck.py $out/${pkgs.python3.sitePackages}/
            cp src/libtsduck/python/ts.py $out/${pkgs.python3.sitePackages}/
            runHook postInstall
          '';

          postFixup =
            let
              libPath = "${tsduckPkg}/lib/libtsduck.${if pkgs.stdenv.isDarwin then "dylib" else "so"}";
            in ''
              substituteInPlace $out/${pkgs.python3.sitePackages}/tsduck.py \
                --replace-fail "return ctypes.util.find_library('tsduck')" 'return "${libPath}"'
            '';

          pythonImportsCheck = [ "tsduck" "ts" ];

          meta = with pkgs.lib; {
            description = "Python bindings for TSDuck";
            homepage = "https://github.com/tsduck/tsduck";
            license = licenses.bsd2;
          };
        };

        # Java bindings parameterised on which tsduck package they wrap.
        mkJavaTsduck = tsduckPkg: pkgs.stdenv.mkDerivation {
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
          default = self.packages.${system}.tsduck;

          # Third-party hardware support packages (built separately)
          inherit linuxX86Only;

          # TSDuck with all hardware support (default)
          tsduck = pkgs.callPackage ./tsduck.nix {
            inherit dev;
            dtapi = linuxX86Only.dtapi;
            vatek = linuxX86Only.vatek;
            enableHides = true;
          };

          tsduck-min = pkgs.callPackage ./tsduck.nix {
            inherit dev;
            dtapi = null;
            vatek = null;
            enableHides = false;
          };

          # Python bindings
          python-tsduck     = mkPythonTsduck self.packages.${system}.tsduck;
          python-tsduck-min = mkPythonTsduck self.packages.${system}.tsduck-min;

          # Java bindings
          java-tsduck     = mkJavaTsduck self.packages.${system}.tsduck;
          java-tsduck-min = mkJavaTsduck self.packages.${system}.tsduck-min;
        };

        devShells = {
          default = pkgs.mkShell {
            # Inherit all build dependencies from the full package (with Java)
            inputsFrom = [ self.packages.${system}.tsduck ];

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
