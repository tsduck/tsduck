# Installing TSDuck   {#installing}

TSDuck can be installed on Windows, Linux, macOS and BSD systems (FreeBSD, OpenBSD, NetBSD, DragonFlyBSD).

# Windows {#wininstall}

[Executable binary installers for the latest TSDuck version](https://tsduck.io/download/tsduck)
are available for 64-bit Windows on Intel systems.

All tools, plugins and development environments are in the same installer.
Running the installer provides several options:

- Tools & Plugins
- Documentation
- Python Bindings (optional)
- Java Bindings (optional)
- C++ Development (optional)

[Older versions of TSDuck](https://github.com/tsduck/tsduck/releases) remain available on GitHub.
[Nightly builds and pre-releases](https://tsduck.io/download/prerelease) can be found on the
TSDuck Web site.

To automate the installation, the executable binary installer can be run from the command line
or a script.
- The option `/S` means "silent". No window is displayed, no user interaction is possible.
- The option `/all=true` means install all options. By default, only the tools, plugins and
  documentation are installed. In case of upgrade over an existing installation, the default
  is to upgrade the same options as in the previous installation.

# macOS {#macinstall}

TSDuck is installable on macOS systems using [Homebrew](https://brew.sh),
the package manager for open-source projects on macOS.

If you have never used Homebrew on your system, you can install it using the
following command (which can also be found on the [Homebrew home page](https://brew.sh)):
~~~
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
~~~

Once Homebrew is set up, you can install TSDuck using:
~~~
brew install tsduck
~~~

All tools, plugins and development environments are installed.

After installation, to upgrade to latest version:
~~~
brew update
brew upgrade tsduck
~~~

When Homebrew upgrades packages, the old versions are not removed. The new versions
are just added. After a while, megabytes of outdated packages accumulate on disk.
To remove outdated packages:
~~~
brew cleanup
~~~

To uninstall TSDuck:
~~~
brew uninstall tsduck
~~~

If you would like to install the lastest test version (HEAD version) use
the following command. Be aware that it takes time since TSDuck is
locally recompiled.
~~~
brew install --HEAD tsduck
~~~

# Linux {#linuxinstall}

[Pre-build packages for the latest TSDuck version](https://tsduck.io/download/tsduck)
are available for the following configurations:

- Fedora (64-bit Intel)
- Ubuntu (64-bit Intel)
- RedHat, CentOS, Alma Linux (64-bit Intel)
- Debian (64-bit Intel)
- Raspbian (32-Bit Arm, Raspberry Pi)

The type of package, `.rpm` or `.deb`, depends on the configuration.
The pre-built packages are provided for the latest version of each distro only.

For each distro, two packages exist: the `tsduck` package installs
the TSDuck commands, plugins, Java and Python bindings,
the `tsduck-devel` or `tsduck-dev` package installs the development
environment for C++ programmers.

[Older versions of TSDuck](https://github.com/tsduck/tsduck/releases) remain available on GitHub.
[Nightly builds and pre-releases](https://tsduck.io/download/prerelease) for Ubuntu
can be found on the TSDuck Web site.

To use older versions of the above distros, rebuilding the packages is easy:
~~~
make -j10
make installer
~~~

To install TSDuck on other types of Linux systems for which no package
is available,
~~~
make -j10
sudo make install
~~~

More details on how to build TSDuck are [available here](building.html).

# BSD systems (FreeBSD, OpenBSD, NetBSD, DragonFlyBSD) {#bsdinstall}

There is currently no installer for FreeBSD, OpenBSD, NetBSD, DragonFlyBSD.
You need to build and install as follow:
~~~
gmake -j10
sudo gmake install
~~~

Note that GNU Make (`gmake`) shall be used instead of the standard BSD `make`.
