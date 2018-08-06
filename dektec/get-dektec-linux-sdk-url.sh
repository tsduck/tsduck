#!/bin/bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2018, Thierry Lelegard
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#  This script retrieves the URL of the latest LinuSDK from Dektec.
#
#-----------------------------------------------------------------------------

URL_BASE=https://www.dektec.com
HTML_URL=$URL_BASE/downloads/SDK/
GENERIC_URL=$URL_BASE/products/SDK/DTAPI/Downloads/LatestLinuxSDK

SCRIPT=$(basename $BASH_SOURCE)
error() { echo >&2 "$SCRIPT: $*"; exit 1; }

# Merge an URL with its base.
merge_url()
{
    local ref="$1"
    local url
    read url

    if [[ -n "$url" ]]; then
        if [[ $url == *:* ]]; then
            echo "$url"
        elif [[ $url == /* ]]; then
            echo "$URL_BASE$url"
        elif [[ $ref == */ ]]; then
            echo "$ref$url"
        else
            ref=$(dirname "$ref")
            echo "$ref/$url"
        fi
    fi
}

# Retrieve the URL using the redirection from a fixed generic URL.
# This should be the preferred method but Dektec may forget to update
# the redirection in the generic URL.
from_generic_url()
{
    curl --silent --show-error --dump-header /dev/stdout "$GENERIC_URL" | \
        grep -i 'Location:' | \
        sed -e 's/.*: *//' -e 's/\r//g' | \
        merge_url "$GENERIC_URL"
}

# Retrieve the URL by parsing the HTML from the Dektec download web page.
from_html_page()
{
    curl --silent --show-error --location "$HTML_URL" | \
        grep 'href=".*LinuxSDK' | \
        sed -e 's/.*href="//' -e 's/".*//' | \
        merge_url "$HTML_URL"
}

# Try the HTML parsing first, then redirection.
URL=$(from_html_page)
[[ -z "$URL" ]] && URL=$(from_generic_url)
[[ -z "$URL" ]] && error "cannot locate LinuxSDK location from Dektec"

echo "$URL"
exit 0
