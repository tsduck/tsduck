# Grafana test environment

This directory contains tools to setup a minimal Grafana environment to test
the plugin `influx`.

**Contents:**

* [Grafana setup](#grafana-setup)
  * [Demo installation](#demo-installation)
  * [Testing InfluxDB](#testing-influxdb)
  * [Configuring InfluxDB as data source for Grafana](#configuring-influxdb-as-data-source-for-grafana)
  * [Creating Grafana dashboards](#creating-grafana-dashboards)
  * [Importing the sample dashboard](#importing-the-sample-dashboard)
* [InfluxDB CLI](#influxdb-cli)
* [Docker on macOS](#docker-on-macos)
* [References](#references)

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

### Demo installation

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
  Defining these environment variables is not mandatory. It is just an easy
  way to avoid retyping all access parameters to access the InfluxDB server
  in each command.

After setup, Grafana and InfluxDB can be managed from a browser at:

- Grafana: http://localhost:3000
- InfluxDB: http://localhost:8086

The user name is `admin` in both cases. See the initial default passwords in
the file `docker-compose.yml`.

### Testing InfluxDB

This step is optional. It is just a test. It requires the presence of the
InfluxDB CLI command `influx` ([see below](#influxdb-cli)).

First, run a `tsp` command which logs metrics in InfluxDB. Note that sourcing
`setenv.sh` defines the environment variables which equally apply to the `tsp`
plugin `influx` and the InfluxBD CLI command `influx`.
~~~
source setenv.sh
tsp -b 1,000,000 -I null -P regulate -P influx -O drop
~~~

Then, from another shell, query the received data for the last 20 seconds.
Keep in mind that the display requires a very wide terminal window.
~~~
source setenv.sh
influx query 'from(bucket:"demo-bucket") |> range(start:-20s)'
~~~

If the tabular data are displayed, InfluxDB and TSDuck are correctly connected.

### Configuring InfluxDB as data source for Grafana

Using a browser, connect to Graphana at `http://localhost:3000`. You may also
connect from another system, using the appropriate host name instead of `localhost`.
However, keep in mind that the demo setup is minimal. No TLS certificate was
generated and HTTP is used in the clear (no HTTPS). Therefore, passwords are
transmitted in the clear. This may be fine for a demo setup with known default
passwords but *never* do this in production.

Once connected to Grafana, configure InfluxDB as data source. Depending on the
version, the UI may vary. Adapt the following procedure if necessary.

- In left panel, under "Connections", select "Data sources".
- Select "Add data source".
- Select "InfluxDB".
- Configure InfluxDB:
  - Query language: "Flux", instead of the default "InfluxQL".
  - HTTP URL: "http://influxdb:8086".
    Important: specify "influxdb", literally, not "localhost".
  - Auth: Keep "Basic Auth", which should be already selected.
  - Scroll down to InfluxDB Details.
  - Organization: "demo-org".
  - Token: "demo-token".
  - Default Bucket: "demo-bucket".
  - Click on the blue button "Save & test".
  - You should see a message similar to "datasource is working. 3 buckets found".

Grafana can now pull data to display from InfluxDB.

### Creating Grafana dashboards

Let's create a visualization of the global transport stream bitrate.

Still in the Grafana panel at `http://localhost:3000`:

- In left panel, select "Dashboards".
- Select "Create dashboard".
- Select "Add visualization".
- Under "Select data source", click on "influxdb (default)".
- In the "Queries" tab, enter a Flux query such as:
~~~
from(bucket: "demo-bucket")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r["_measurement"] == "bitrate")
  |> filter(fn: (r) => r.scope == "ts")
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> yield(name: "mean")
~~~
- Select the duration of visualization (top right), for instance "Last 15 minutes".
- Click on "Refresh" (top right) to get an snapshot of data.
- Select "Auto" in the refresh option, as well as a refresh rate.
  The lowest value is 5s and is appropriate to transport stream monitoring.

### Importing the sample dashboard

A Grafana dashboard can be exported and imported as a "JSON Model".

This directory contains a sample dashboard model in the JSON file `sample-graph.json`.
To import it in Grafana, from the Grafana panel at `http://localhost:3000`:

- Select "Dashboards" on the top line (in "Home > Dashboards").
- On the top right corner, select "New", then "Import".
- Select "Upload dashboard JSON file" or drag an drop `sample-graph.json` into that tab.
- Select "Import".
- Possibly set auto-refresh to 5s in top right corner.

This sample graph displays a stacked view of the bitrates of the various data types
in a transport stream (audio, video, subtitles, etc). To inject the corresponding
metrics data in InfluxDB from a sample TS file, use this command:

~~~
tsp -I file -i input.ts -P regulate -P influx --type -O drop
~~~

## InfluxDB CLI

The command `influx` is designed to query an InfluxDB instance. It can be used
to debug the data which are logged by the `tsp` plugin `influx`. The InfluxDB
instance can be identified using command line options or environment variables.
In the demo environment, source the script `setenv.sh` to define the appropriate
environment variables.

The following command dumps the data which were ingested in the last minute:
~~~
influx query 'from(bucket:"demo-bucket") |> range(start:-1m)'
~~~

Note: To install the InfluxDB CLI on macOS, use `brew install influxdb-cli`.

## Docker on macOS

On macOS, use HomeBrew to install Docker. Use the "cask" version, not the
"formula" version. The installation command is `brew install --cask docker`.

Containers are Linux environments. They can only run on top of a Linux
kernel. On macOS, Docker containers run into a basic Linux virtual machine
("linuxkit" kernel with "Docker Desktop" operating system) which is started
as the "Docker Engine". The `docker` command is native to macOS but the
containers run in the Docker Engine.

The easiest way to start the Docker Engine is to start the "Docker Desktop"
macOS application (`/Applications/Docker.app`). It is also possible to start
and stop the Docker Engine from the command line as follow:
~~~
docker desktop start
docker desktop stop
~~~

Note that there are other possible setups for Docker on macOS, using either
other types of Linux environment (Colima) or other types of virtualization
(VirtualBox). The "Docker Desktop" environment is just the easiest one to use.

## References

- [Configure a Grafana Docker image](https://grafana.com/docs/grafana/latest/setup-grafana/configure-docker/)
- [InfluxDB HTTP API](https://docs.influxdata.com/influxdb/v2/api/v2/).
- [InfluxDB "line protocol", used by the `influx` plugin to send data to InfluxDB](https://docs.influxdata.com/influxdb/v2/reference/syntax/line-protocol/).
- [InfluxDB CLI, `influx` command, for debug](https://docs.influxdata.com/influxdb/v2/reference/cli/influx/).
- [Docker Desktop documentation](https://docs.docker.com/desktop/).
