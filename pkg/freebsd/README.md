# Packaging for FreeBSD systems

This directory contains instructions to maintain the status of TSDuck as a
package for FreeBSD systems.

There are lots of FreeBSD online documentations and forums. However, the instructions
are not always clear or consistent. It seems that the FreeBSD ecosystem experimented
successive workflows to manage packages. Several alternative methods seem possible to
achieve the same purpose and developers who are not familiar with FreeBSD are easily
lost. The following instructions describe one possible workflow. It may not be the best
or most recommended one, but is seems to work.

## FreeBSD ports

FreeBSD maintains two sets of installation packages: the "core" operating system and
application packages. The application packages are called "ports". The ports packages
are installed under `/usr/local` and are consequently isolated from the core operating
system. The package manager for FreeBSD ports is the command `pkg`.

Note: This is similar to macOS where the core operating system is isolated, protected
and maintained by Apple, while the open source applications packages are maintained as
part of the "Homebrew" community effort and installed under `/opt/homebrew`, in an
separate environment. On the other hand, this is very different from most Linux distros
where the core operating system, kernel included, is managed by the same package manager
as any standard application.

On FreeBSD, TSDuck is maintained as a FreeBSD port, in the multimedia category. The
package name is therefore `multimedia/tsduck`.

TSDuck is installed on FreeBSD system using the simple command `pkg install tsduck`,
as described in the [developer's guide](https://tsduck.io/docs/tsduck-dev.html#freebsdinstall).

## Updating the FreeBSD port for TSDuck

The set of FreeBSD ports is maintained by FreeBSD "committers" in a git repository here:

- https://git.freebsd.org/ports.git

Each "port" consists in a set of files - a Makefile and a few description files -
in a dedicated directory, in the corresponding category. The TSDuck port can be
viewed online at:

- https://cgit.freebsd.org/ports/tree/multimedia/tsduck

The initial port for TSDuck was developed by an experienced FreeBSD committer.
Each time this port needs to be updated (typically when a new version of TSDuck
is released), a TSDuck maintainer shall submit a patch to the FreeBSD ports
maintainers.

### Setting up a FreeBSD system for ports maintenance

The update is typically performed on a FreeBSD system. The initial setup is
described here.

The tree of ports files is documented as being maintained in `/usr/ports`. It is
supposed to be a local clone of the FreeBSD ports git repository. However, the
status of this directory is unclear. On some freshly installed FreeBSD systems,
this directory exists but is empty. On some older FreeBSD systems, this directory
contains a copy of the ports files, but it is not a git repository.

So, let's fix this. First, take ownership of `/usr/ports`. It is initially owned
by root and superuser access is needed to modify it. Since it is a very bad idea
to build software using the root account, change its ownership, recursively.
It the directory exists but is not a git repository (no `.git` subdirectory),
delete its content and clone the repository.

~~~
sudo chown -R <username> /usr/ports
git clone --depth=1 -b main https://git.freebsd.org/ports.git /usr/ports
~~~

Note that we limit the scope of the clone (main branch only, current state only,
without history) to limit the storage usage.

### Updating the TSDuck port

To work on an update of the TSDuck port, first create a specific git branch,
for instance `tsduck-update`:

~~~
git checkout -b tsduck-update
~~~

Then, modify the files in the TSDuck port, typically the Makefile.

To build and test the port:

~~~
make install cleanup -C /usr/ports/multimedia/tsduck
~~~

Use `make reinstall` if TSDuck is already installed.

Once the modifications are ready, commit them and create the corresponding patch,
containing the differences between the local branch and the `main` branch.

~~~
git add .
git commit
git format-patch main
~~~

The command `git format-patch` creates a file named `0001-commit-title.patch` in
the current directory. Use option `-o` to create it elsewhere.

Finally, open a PR (problem report) on the FreeBSD bugzilla here:

- https://bugs.freebsd.org/

Use product "Ports & Packages", component "Individual Port(s)". Provide the
created patch file as attachment. Clearly describe the changes. The PR will
be processed by FreeBSD committers and they will apply the patch.

## Quarterly and latest ports

As a final note, new and updated FreeBSD ports are made available in two waves:
"latest" and "quarterly". As the names imply, the "quarterly" repository is updated
every three months only. This is the default. If you want to install ports which
were recently updated, without waiting for the next quarter, you have to switch
to the "latest" branch.

To reconfigure the `pkg` command to download packages from the "latest" branch,
update the file `/usr/local/etc/pkg/repos/FreeBSD.conf` as follow:

~~~
FreeBSD: {
    url: "pkg+https://pkg.FreeBSD.org/${ABI}/latest",
    enabled : yes,
    mirror_type : "SRV"
}
~~~
