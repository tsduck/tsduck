#!/usr/bin/env bash
#
# Setup the Grafana demo environment using the file docker-compose.yml
# in the same directory as this script.
#
cd $(dirname $0)
mkdir -p data/grafana data/influxdb
chmod 777 data/grafana data/influxdb
docker-compose up -d
