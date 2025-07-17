#!/usr/bin/env bash
#
# Setup a test key and certificate.
# If source'd from bash, the appropriate environment variables are defined.
#

# Key location:
_keydir="$HOME/tmp"
_certfile="$_keydir/test_cert.pem"
_keyfile="$_keydir/test_key.pem"
[[ -d "$_keydir" ]] || mkdir -p "$_keydir"

# Create key and certificate.
if [[ ! -f "$_certfile" || ! -f "$_keyfile" ]]; then
    openssl req -quiet -newkey rsa:3072 -new -noenc -x509 -subj="/CN=$(hostname)" -days 3650 -keyout "$_keyfile" -out "$_certfile"
    ls -l "$_keyfile" "$_certfile"
fi

# Define environment variables for default values.
export TSDUCK_TLS_CERTIFICATE="$_certfile"
export TSDUCK_TLS_KEY="$_keyfile"
