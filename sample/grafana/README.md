# Grafana test environment

This directory contains tools to setup a minimal Grafana environment to test
the plugin `influx`.

**Contents:**

* [Grafana setup](#grafana-setup)
* [Docker on macOS](#docker-on-macos)

Grafana is a generic visualization framework which is now a de-facto standard.
Grafana pulls the data to visualize from various "data sources". For real-time
monitoring, the most common data source is "InfluxDB". Applications send data
to InfluxDB using a Web API. These data are stored in InfluxDB, with a given
retention duration. Grafana polls new information from InfluxDB for display.

TSDuck can send real-time bitrate monitoring information to InfluxDB using the
plugin `influx`. This information can be used to build various monitoring graphs
using Grafana.

## Grafana setup

Setting up Grafana and its data sources is not trivial. If you do not have a
ready-to-use Grafana test or production environment, this directory contains
scripts to setup and cleanup a minimal Grafana demo environment.

Grafana and InfluxDB are installed in Docker containers to avoid interferences
with the rest of the system. The only prerequisites are the installation of
Docker, the `docker` and `docker-compose` commands. On macOS, see
[Docker on macOS](#docker-on-macos) below.

In this setup, Grafana and InfluxDB run in distinct containers, named `grafana`
and `influxdb`, respectively.

Setup files:

- `docker-compose.yml`: Docker configuration for Grafana and InfluxDB.
- `grafana-setup.sh`: Create the demo environment using `docker-compose.yml`.
- `grafana-cleanup.sh`: Cleanup and delete the demo environment.
- `setenv.sh`: A script to source in bash, which defines the environment
  variables to access the InfluxDB instance from the `tsp` plugin `influx`.

After setup, Grafana and InfluxDB can be managed from a browser at:

- Grafana: http://localhost:3000
- InfluxDB: http://localhost:8086

The user name is `admin` in both cases. See the initial default passwords in
the file `docker-compose.yml`.

## Docker on macOS

On macOS, use HomeBrew to install Docker. Use the "cask" version, not the
"formula" version. The installation command is `brew install --cask docker`.

Containers are Linux environments. They can only run on top of a Linux
kernel. On macOS, Docker containers run into a basic Linux virtual machine
("linuxkit") which is started as the "Docker Engine". The `docker` command
is native to macOS but the containers are in the Docker Engine. The easiest
way to start the Docker Engine is to start the "Docker Desktop" macOS
application (`/Applications/Docker.app`).

To install the InfluxDB CLI, use `brew install influxdb-cli`.

## References

- [InfluxDB HTTP API](https://docs.influxdata.com/influxdb/v2/api/v2/).
- [InfluxDB "line protocol", used by the `influx` plugin to send data to InfluxDB](https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/).
- [InfluxDB CLI, `influx` command, for debug](https://docs.influxdata.com/influxdb/v2/reference/cli/influx/).
