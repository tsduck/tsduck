# Winget packaging for Windows

Winget is the new modern package manager for open-source software on Windows.
It is somewhat similar to HomeBrew on macOS, with some differences.

The exact status of winget is not fully understood. It is maintained under
the Microsoft account on GitHub and anyone can submit new packages or updates
of existing packages. More details on winget can be found
[here](https://learn.microsoft.com/en-us/windows/package-manager/winget/).

Similarities with HomeBrew on macOS:

- Command line based tool (`winget install <packagename>`, `winget upgrade`, etc.)
- Package repository on GitHub ([microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs)).
- A package is described using a _manifest_ in that repository (a _formula_ in HomeBrew).
- Anyone can publish a new or updated package using a pull request on the package repository.

Differences with HomeBrew on macOS:

- Maintained by Microsoft instead of a community.
- Winget packages are binary while HomeBrew packages are source.
  - With winget, the contributor shall build the packages for the various architectures
    and list all binary packages in the manifest.
  - With HomeBrew, the contributor provides a link to a source tarball and build commands
    in the formula. The HomeBrew infrastructure rebuilds the binaries for all supported
    architectures and operating system versions.
- Multiple versions of the same winget package are concurrently available. Each version has
  its own manifest (a subdirectory named from the version). With HomeBrew, the formula for
  a new version replaces the previous formula.

Instructions to submit a new package are
[documented here](https://learn.microsoft.com/en-us/windows/package-manager/package/repository).

A package is identified by the provider and software name. TSDuck is identified as `TSDuck.TSDuck`.

The winget manifest for TSDuck can be found here:
https://github.com/microsoft/winget-pkgs/tree/master/manifests/t/TSDuck/TSDuck

Because winget manages binary packages only, the NSIS installer for TSDuck shall be
built first and published on GitHub as a release. Then, the winget manifest for TSDuck
will point to that release on GitHub.

As of this writing, the current release of TSDuck is 3.35. This is the first version
which was submitted to winget.

Submitting an update has not yet been done. It is unclear if new releases of packages on GitHub are
automatically detected, as [discussed here](https://github.com/microsoft/winget-cli/discussions/2525)
or if an explicit pull request must be submitted.

The command `wingetcreate update` also seems to provide some assistance in the publication of package updates.
It is [described here](https://github.com/microsoft/winget-create/blob/main/doc/update.md).

This README file will be updated and potential automation scripts will be placed
in this directory when TSDuck version 3.36 is released.
