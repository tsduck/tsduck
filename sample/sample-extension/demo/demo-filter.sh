#!/usr/bin/env bash
# This script demonstrates the impact of the "foo" section filter on tstables.

cd $(dirname $0)

tstables demo.ts -o demo.tstables1.txt --no-duplicate --tid 0xF0 --tid 0x80-0x8F
tstables demo.ts -o demo.tstables2.txt --no-duplicate --tid 0xF0 --tid 0x80-0x8F --foo-id 0x123
tstables demo.ts -o demo.tstables3.txt --no-duplicate --tid 0xF0 --tid 0x80-0x8F --foo-id 12
