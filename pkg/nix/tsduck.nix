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
  git,
  glibcLocales,
  which,
  jdk,
  libedit,
  librist,
  libusb1,
  openssl,
  pcsclite,
  srt,
  zlib,
  # Optional hardware support packages
  dtapi ? null,
  vatek ? null,
  # Enable HiDes support (kernel drivers, no separate package needed)
  enableHides ? false,
  # Development build flag (set by flake based on git dirty status)
  dev ? true,
}:

let
  # Extract version from tsVersion.h
  versionHeader = builtins.readFile ../../src/libtscore/tsVersion.h;
  versionMajor = builtins.head (builtins.match ".*#define TS_VERSION_MAJOR ([0-9]+).*" versionHeader);
  versionMinor = builtins.head (builtins.match ".*#define TS_VERSION_MINOR ([0-9]+).*" versionHeader);
  commit = builtins.head (builtins.match ".*#define TS_COMMIT ([0-9]+).*" versionHeader);
  baseVersion = "${versionMajor}.${versionMinor}-${commit}";

  # Append -dev suffix for development builds
  version = baseVersion + (if dev then "-dev" else "");
in

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
    curl.dev  # curl-config (in the dev output) must be on PATH for make-config.sh detection
    doxygen
    graphviz
    jdk
    python3
    ruby
    qpdf
    which  # needed for make-config.sh uses 'which' to probe for curl-config
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
    "NOGITHUB=1"
    "SYSPREFIX=/"
    "SYSROOT=${placeholder "out"}"
    "PCSC_DONE=1"
    "RIST_DONE=1"
    "SRT_DONE=1"
    "CXXFLAGS_EXTRA=-I${pcsclite.dev}/include/PCSC"
  ]
  # Conditionally disable hardware support if packages not provided
  ++ lib.optional (dtapi == null) "NODEKTEC=1"
  ++ lib.optional (vatek == null) "NOVATEK=1"
  ++ lib.optional (!enableHides) "NOHIDES=1"
  # If hardware packages are provided, set their paths
  ++ lib.optionals (dtapi != null) [
    "DTAPI_HEADER=${dtapi}/include/DTAPI.h"
    "DTAPI_OBJECT=${dtapi}/lib/DTAPI64.o"
    "DTAPI_DONE=1"
  ]
  ;

  makeFlagsArray = [
    # Suppress warnings that fire on generated / third-party headers.
    # "CXXFLAGS_NO_WARNINGS=-Wno-deprecated-declarations -Wno-nrvo -Wno-ms-bitfield-padding -Wimplicit-int-conversion -Wno-implicit-int-conversion-on-negation"
  ];

  VATEK_CFLAGS = lib.optionalString (vatek != null) "-isystem ${vatek}/include/vatek";
  VATEK_LDLIBS = lib.optionalString (vatek != null) "${vatek}/lib/libvatek_core.a -lusb-1.0";

  # Don't run full test suite during build (requires network, external resources)
  doCheck = false;

  # Instead, run simple install checks to verify tools are working
  doInstallCheck = true;
  installCheckPhase = ''
    runHook preInstallCheck

    # Test that basic tools run and report version
    $out/bin/tsversion --version=short
    $out/bin/tsp --version=short

    # Test that libraries are found
    $out/bin/tsversion --support=all

    runHook postInstallCheck
  '';

  installTargets = [
    "install-tools"
    "install-devel"
  ];

  passthru = { };

  meta = {
    description = "MPEG Transport Stream Toolkit";
    homepage = "https://github.com/tsduck/tsduck";
    mainProgram = "tsversion";
    license = lib.licenses.bsd2;
    maintainers = with lib.maintainers; [ siriobalmelli ];
    platforms = lib.platforms.all;
  };
})
