{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  libusb1,
}:

# VATek SDK - SDK for VATek modulator chips

stdenv.mkDerivation rec {
  pname = "vatek-core";
  version = "3.12.1";

  src = fetchFromGitHub {
    owner = "VisionAdvanceTechnologyInc";
    repo = "vatek_sdk_2";
    rev = "v${version}";
    hash = "sha256-25Se/nL5vruoOy5mvcpDrhhJj6Wp8EuX/rMqzjcSE+8=";
  };

  nativeBuildInputs = [
    cmake
  ];

  buildInputs = [
    libusb1
  ];

  cmakeFlags = [
    # Disable Qt, apps, and samples
    "-DSDK2_EN_QT=OFF"
    "-DSDK2_EN_APP=OFF"
    "-DSDK2_EN_SAMPLE=OFF"
    # Build static library only
    "-DSDK2_EN_STATIC_ONLY=ON"
  ];

  env.NIX_CFLAGS_COMPILE = toString [
    "-Wno-error=incompatible-pointer-types"
    "-Wno-incompatible-pointer-types"
  ];

  meta = with lib; {
    description = "VATek SDK - Library for VATek modulator devices";
    homepage = "https://github.com/VisionAdvanceTechnologyInc/vatek_sdk_2";
    license = licenses.bsd2;
    platforms = platforms.linux ++ platforms.darwin;
    maintainers = with maintainers; [ ];
  };

  passthru = {
    includeDir = "${placeholder "out"}/include/vatek";
    staticLib = "${placeholder "out"}/lib/libvatek_core.a";
  };
}
