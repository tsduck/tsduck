#!/usr/bin/env bash
#
# Cleanup the Grafana demo environment which was set by grafana-setup.sh.
# This is a full cleanup. The containers and their volumes are deleted.
#
cd $(dirname $0)
for cont in grafana influxdb; do
    docker ps | grep -qw $cont && docker stop $cont
    docker ps -a | grep -qw $cont && docker rm $cont    
done
# Try to delete the data without privilege.
rm -rf data 2>/dev/null
# May need sudo assistance if some files remain
# (the containers may create files which are owned by 'root').
[[ -d data ]] && sudo rm -rf data
# Always exit with success status.
exit 0
