## Linux implementation of tuner interface

All headers for the LinuxTV API (a.k.a. DVB API) are included in `tsDTVProperties.h`.
A synthetic value `TS_DVB_API_VERSION` is built here.
Example: `TS_DVB_API_VERSION` is 503 for DVB API version 5.3.

The LinuxTV API has evolved a lot. Be sure to condition the usage of new
features with the value of `TS_DVB_API_VERSION`.

The script `get-linux-tv-api-history.sh` uses the Linux kernel git repository
to build a history of all versions of the Linux TV API.
