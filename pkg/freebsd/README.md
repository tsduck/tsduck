# Packaging for FreeBSD systems

This directory contains instructions to maintain the status of TSDuck as a
package for FreeBSD systems.

There are lots of FreeBSD online documentations and forums. However, the instructions
are not always clear or consistent. It seems that the FreeBSD ecosystem experimented
successive workflows to manage packages. Several alternative methods seem possible to
achieve the same purpose and developers who are not familiar with FreeBSD are easily
lost. The following instructions describe one possible workflow. It may not be the best
or most recommended one, but is seems to work on FreeBSD 15.0.

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

TSDuck is installed on FreeBSD systems using the simple command `pkg install tsduck`,
as described in the [developer's guide](https://tsduck.io/docs/tsduck-dev.html#freebsdinstall).

## Updating the FreeBSD port for TSDuck

The set of FreeBSD ports is maintained by FreeBSD authorized committers in a git
repository here:

- https://git.freebsd.org/ports.git

Each port consists in a set of files - a Makefile and a few description files -
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

So, let's fix this. First, if the directory exists but is not a git repository
(no `.git` subdirectory), delete its content. Second, clone the repository:

~~~
git clone --depth=1 -b main https://git.freebsd.org/ports.git /usr/ports
~~~

Note that we limit the scope of the clone (main branch only, current state only,
without history) to limit the storage usage.

Security note: `/usr/ports` is owned by root and superuser access is required
to modify it. It is usually agreed that building software, and more generally
performing non-admin tasks, using the root account is a very bad idea for
security reasons. However, even if we change the ownership of `/usr/ports`
and run the build operations using a standard user account, some tasks in
the port build process still require root privileges. As a consequence, we
keep the standard configuration and all commands which are described below
are run using the root account. It is recommended to use a disposable FreeBSD
system to work on ports to avoid security issues.

### Updating the TSDuck port

To work on an update of the TSDuck port, first create a specific git branch,
for instance `tsduck-update`:

~~~
cd /usr/ports/multimedia/tsduck
git checkout -b tsduck-update
~~~

Then, modify the files in the TSDuck port.

For a new version of TSDuck, update the `Makefile` with the new version.

- Update variables `DISTVERSION` and `DISTVERSIONSUFFIX`.
- The variable `PORTREVISION` shall be removed or set to 0. After publication,
  if the port needs to be fixed for the same version of TSDuck, set `PORTREVISION`
  to 1 and increment it for each fix of that port.
- If the new version of TSDuck introduces new dependencies, adjust the
  corresponding variables.

Delete the directory `files` if it exists (`/usr/ports/multimedia/tsduck/files`).
It contains patches to apply to that version of TSDuck for FreeBSD. It that
directory exists, the patches apply to the previous version. So, remove it.

~~~
rm -rf files
~~~

Regenerate the `distinfo` file. It contains the description and checksum
of the source archive.

~~~
rm distinfo
make makesum
~~~

If necessary, the `makesum` target also downloads the source archive for
the new version of TSDuck in `/usr/ports/distfiles`.

Build TSDuck and install it into an intermediate "stage" location.

~~~
make stage
~~~

This command takes time. It rebuilds TSDuck from sources and installs it in
the subdirectory `work/stage/usr/local`. Review that the build generates no
error and the files are correctly installed.

Now, regenerate the file `pkg-plist`. It contains the list of all files which
are installed by the package.

~~~
make makeplist >pkg-plist
~~~

*Important:* This command generates a file which needs to be reviewed and
adjusted. To enforce that, its first line contains the fake file path
`/you/have/to/check/what/makeplist/gives/you`. Usually, reviewing
`pkg-plist` simply means removing that fake first line.

To test a real installation of the package:

~~~
make install
~~~

Use `make reinstall` if TSDuck is already installed.

To cleanup the work area:

~~~
make clean
~~~

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
