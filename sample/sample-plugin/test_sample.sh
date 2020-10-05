#!/usr/bin/env bash
# Test script for the sample plugin.

# In this test, we assume that the plugin was built in the same directory as this script.
export TSPLUGINS_PATH=$(dirname $0):$TSPLUGINS_PATH

# Test the sample plugin.
tsp -I null -P until --packet 200 -P sample --count -O drop
