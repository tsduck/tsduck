#!/usr/bin/env bash
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2026, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Cleanup the image and all instances of the tsduck container.
#
#-----------------------------------------------------------------------------

for cont in $(docker ps -a | awk '{print $1 " " $2}' | grep -e 'tsduck$' | awk '{print $1}'); do
    docker container rm $cont
done

docker image rm tsduck
