{
  lib,
  stdenv,
  # build and doc tooling
  asciidoctor-with-extensions,
  doxygen,
  graphviz,
  python3,
  ruby,
  qpdf,
  udevCheckHook ? null,
  # build deps
  curl,
  editline,
  glibcLocales,
  jdk,
  libedit,
  librist,
  libusb1,
  openssl,
  pcsclite,
  srt,
  zlib,
  # version from flake or default
  version ? "dev",
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "tsduck";
  inherit version;

  src = lib.fileset.toSource {
    root = ../..;  # Repository root (two levels up from pkg/nix/)
    fileset = lib.fileset.unions [
      ../../src
      ../../scripts
      ../../images
      ../../doc
      ../../Makefile
      ../../Makefile.inc
      ../../CONFIG.txt
      ../../LICENSE.txt
      ../../CHANGELOG.txt
      ../../CONTRIBUTORS.txt
      ../../OTHERS.txt
      ../../README.md
    ];
  };
  nativeBuildInputs = [
    asciidoctor-with-extensions
    doxygen
    graphviz
    jdk  # Build-time only, not a runtime dependency
    python3
    ruby
    qpdf
  ] ++ lib.optionals (stdenv.isLinux && udevCheckHook != null) [
    udevCheckHook
  ];

  buildInputs = [
    curl
    editline
    libedit
    librist
    libusb1
    openssl
    srt
    zlib
  ] ++ lib.optionals stdenv.isLinux [
    glibcLocales
    pcsclite
  ] ++ lib.optionals stdenv.isDarwin [
    # On macOS, PCSC is provided by the system framework
  ];

  enableParallelBuilding = true;

  postPatch = ''
    patchShebangs scripts
  '';

  # see CONFIG.txt in the sources
  makeFlags = [
    "CXXFLAGS_NO_WARNINGS=-Wno-deprecated-declarations"
    "NODEKTEC=1"
    "NOGITHUB=1"
    "NOHIDES=1"
    "NOVATEK=1"
    "NOEXTERNALTESTS=1"
    "SYSPREFIX=/"
    "SYSROOT=${placeholder "out"}"
    "RIST_DONE=1"
    "SRT_DONE=1"
  ];

  checkTarget = "test";
  doCheck = true;
  doInstallCheck = false;

  installTargets = [
    "install-tools"
    "install-devel"
  ];

  passthru = {
    # Python bindings for TSDuck
    python = python3.pkgs.buildPythonPackage {
      pname = "tsduck";
      inherit version;
      format = "other";  # No setup.py or pyproject.toml

      src = lib.fileset.toSource {
        root = ../..;
        fileset = lib.fileset.unions [
          ../../src/libtsduck/python/tsduck.py
          ../../src/libtsduck/python/ts.py
        ];
      };

      # Depend on TSDuck library
      propagatedBuildInputs = [ finalAttrs.finalPackage ];

      # No build phase needed - pure Python
      dontBuild = true;

      installPhase = ''
        runHook preInstall

        mkdir -p $out/${python3.sitePackages}
        cp src/libtsduck/python/tsduck.py $out/${python3.sitePackages}/
        cp src/libtsduck/python/ts.py $out/${python3.sitePackages}/

        runHook postInstall
      '';

      # Patch the library search to use Nix store path
      postFixup =
        let
          libPath = "${finalAttrs.finalPackage}/lib/libtsduck.${if stdenv.isDarwin then "dylib" else "so"}";
        in ''
          substituteInPlace $out/${python3.sitePackages}/tsduck.py \
            --replace-fail "return ctypes.util.find_library('tsduck')" 'return "${libPath}"'
        '';

      # Basic tests - check both tsduck and ts (backward compatibility alias)
      pythonImportsCheck = [ "tsduck" "ts" ];

      meta = with lib; {
        description = "Python bindings for TSDuck";
        homepage = "https://github.com/tsduck/tsduck";
        license = licenses.bsd2;
        maintainers = with maintainers; [ siriobalmelli ];
      };
    };

    # Java bindings for TSDuck (only if built with Java support)
    java = stdenv.mkDerivation {
      pname = "tsduck-java";
      inherit version;

      # No source needed - we copy from the main tsduck build
      dontUnpack = true;
      dontBuild = true;

      # Runtime dependency on JDK and TSDuck library
      propagatedBuildInputs = [ jdk finalAttrs.finalPackage ];

      installPhase = ''
        runHook preInstall

        mkdir -p $out/share/java
        cp ${finalAttrs.finalPackage}/share/tsduck/java/tsduck.jar $out/share/java/

        # Create a convenience symlink
        ln -s $out/share/java/tsduck.jar $out/share/java/tsduck-${version}.jar

        runHook postInstall
      '';

      meta = with lib; {
        description = "Java bindings for TSDuck";
        homepage = "https://github.com/tsduck/tsduck";
        license = licenses.bsd2;
        maintainers = with maintainers; [ siriobalmelli ];
      };
    };
  };

  meta = {
    description = "MPEG Transport Stream Toolkit";
    homepage = "https://github.com/tsduck/tsduck";
    mainProgram = "tsversion";
    license = lib.licenses.bsd2;
    maintainers = with lib.maintainers; [ siriobalmelli ];
    platforms = lib.platforms.all;
  };
})
