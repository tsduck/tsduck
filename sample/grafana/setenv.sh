#
# This script shall be source'd from bash to define the InfluxDB environment
# variables which are used by the tsp plugin influx (and also the InfluxDB CLI
# command "influx").
#
# Without these environment variables, the following parameters shall be used:
# tsp -P influx --host-url http://localhost:8086/ --org demo-org --token demo-token --bucket demo-bucket
#
export INFLUX_HOST=http://localhost:8086/
export INFLUX_ORG=demo-org
export INFLUX_TOKEN=demo-token
export INFLUX_BUCKET_NAME=demo-bucket
