#!/usr/bin/env bash
# Run a fake ECMG for the purpose of the demo.
# Any real ECMG from a real Conditional Access System would be fine.

tsecmg "$@" --port 4567 --verbose --log-protocol=verbose --log-data=debug
