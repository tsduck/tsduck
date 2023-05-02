#!/usr/bin/env bash
#
# Test effectiveness of accelerated instructions on crypto algorithms.
# The utest exe shall be in the PATH.

SCRIPT=$(basename $BASH_SOURCE)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Make sure the compiled libtsduck is used with utest.
UTEST=$(which utest)
[[ -z "$UTEST" ]] && error "utest not found"
BINDIR=$(dirname "$UTEST")
export LD_LIBRARY_PATH="$BINDIR:LD_LIBRARY_PATH"
[[ $(uname -s) == Darwin ]] && export DYLD_LIBRARY_PATH="$BINDIR:DYLD_LIBRARY_PATH"

head='--------------------------------------------------------------------------------'

echo "$head"
echo "Crypto test in default configuration"
echo "$head"
"$BINDIR/tsversion" --version=acceleration
"$BINDIR/utest" -t Crypto

echo "$head"
echo "Crypto test with TS_NO_HARDWARE_ACCELERATION=true"
echo "$head"
TS_NO_HARDWARE_ACCELERATION=true "$BINDIR/tsversion" --version=acceleration
TS_NO_HARDWARE_ACCELERATION=true "$BINDIR/utest" -t Crypto

echo "$head"
echo "CRC32 test in default configuration"
echo "$head"
TSUNIT_CRC32_ITERATIONS=1000000 "$BINDIR/utest" -d -t CRC32

echo "$head"
echo "CRC32 test with TS_NO_HARDWARE_ACCELERATION=true"
echo "$head"
TSUNIT_CRC32_ITERATIONS=1000000 TS_NO_HARDWARE_ACCELERATION=true "$BINDIR/utest" -d -t CRC32

echo "$head"
echo "AES test in default configuration"
echo "$head"
TSUNIT_AES_ITERATIONS=200000 "$BINDIR/utest" -d -t Crypto::AES

echo "$head"
echo "AES test with TS_NO_HARDWARE_ACCELERATION=true"
echo "$head"
TSUNIT_AES_ITERATIONS=200000 TS_NO_HARDWARE_ACCELERATION=true "$BINDIR/utest" -d -t Crypto::AES

echo "$head"
echo "SHA-1 test in default configuration"
echo "$head"
TSUNIT_SHA1_ITERATIONS=10000000 "$BINDIR/utest" -d -t Crypto::SHA1

echo "$head"
echo "SHA-1 test with TS_NO_HARDWARE_ACCELERATION=true"
echo "$head"
TSUNIT_SHA1_ITERATIONS=10000000 TS_NO_HARDWARE_ACCELERATION=true "$BINDIR/utest" -d -t Crypto::SHA1

echo "$head"
echo "SHA-256 test in default configuration"
echo "$head"
TSUNIT_SHA256_ITERATIONS=10000000 "$BINDIR/utest" -d -t Crypto::SHA256

echo "$head"
echo "SHA-256 test with TS_NO_HARDWARE_ACCELERATION=true"
echo "$head"
TSUNIT_SHA256_ITERATIONS=10000000 TS_NO_HARDWARE_ACCELERATION=true "$BINDIR/utest" -d -t Crypto::SHA256

echo "$head"
echo "SHA-512 test in default configuration"
echo "$head"
TSUNIT_SHA512_ITERATIONS=10000000 "$BINDIR/utest" -d -t Crypto::SHA512

echo "$head"
echo "SHA-512 test with TS_NO_HARDWARE_ACCELERATION=true"
echo "$head"
TSUNIT_SHA512_ITERATIONS=10000000 TS_NO_HARDWARE_ACCELERATION=true "$BINDIR/utest" -d -t Crypto::SHA512
