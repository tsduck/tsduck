{
  lib,
  stdenv,
  fetchurl,
  autoPatchelfHook,
}:

# DekTec DTAPI - Professional broadcast hardware interface SDK
# Only available for Linux x86_64

stdenv.mkDerivation rec {
  pname = "dtapi";
  version = "2026.02.0";

  # This url and hash are updated using ./update-hw-sdks.sh
  src = fetchurl {
    url = "https://www.dektec.com/products/SDK/DTAPI/Downloads/LinuxSDK_v2026.02.0.tar.gz";
    hash = "sha256-OPkwQCAQgs6bUQnDhrJXhQaOJNtCb5IGVAoDckg2s6U=";
  };

  meta = {
    description = "DekTec DTAPI - SDK for DekTec broadcast devices";
    homepage = "https://www.dektec.com/products/SDK/DTAPI/";
    # license = lib.licenses.unfree;
    platforms = lib.platforms.linux;
    # Only x86/x86_64 with glibc
    badPlatforms = lib.platforms.darwin ++ [ "aarch64-linux" "armv7l-linux" ];
  };

  # Don't try to build on unsupported platforms
  # Check if we're on Alpine (musl)
  dontBuild = true;
  dontConfigure = true;

  nativeBuildInputs = lib.optionals stdenv.isLinux [
    autoPatchelfHook
  ];

  # The SDK comes as a tarball with this structure:
  # LinuxSDK/
  #   DTAPI/
  #     Include/
  #       DTAPI.h
  #     Lib/
  #       GCC*/
  #         DTAPI.o or DTAPI64.o

  installPhase = ''
    runHook preInstall

    # Create output directories
    mkdir -p $out/include
    mkdir -p $out/lib

    # Find and copy the header
    if [ -f LinuxSDK/DTAPI/Include/DTAPI.h ]; then
      cp LinuxSDK/DTAPI/Include/DTAPI.h $out/include/
    elif [ -f DTAPI/Include/DTAPI.h ]; then
      cp DTAPI/Include/DTAPI.h $out/include/
    else
      echo "Error: Could not find DTAPI.h"
      exit 1
    fi

    # Find and copy the object file
    OBJFILE=""
    if [ "$(uname -m)" = "x86_64" ]; then
      OBJFILE=$(find . -name "DTAPI64.o" | head -1)
    fi
    if [ -n "$OBJFILE" ]; then
      cp "$OBJFILE" $out/lib/
    else
      echo "Error: Could not find DTAPI object file"
      exit 1
    fi

    runHook postInstall
  '';

  # Provide pkg-config-like information for downstream consumers
  passthru = {
    objectFile = "${placeholder "out"}/lib/DTAPI64.o";
    includeDir = "${placeholder "out"}/include";
  };
}
