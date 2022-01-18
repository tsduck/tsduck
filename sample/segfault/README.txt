The scripts in this directory illustrate the following scenario:

- A segmentation fault crash occurs in some specific tsp plugin chain.
- The problem is observed by some remote user but the maintainer needs
  to reproduce the problem locally.
- A very large TS capture file was necessary to reproduce the crash, say
  larger than 100 GB. Since it is not possible to transfer such a large
  file, we want to extract a smaller transferable part of the file which
  reproduces the problem.

The script find-segfault.sh runs the faulty plugin chain on various segments
of the file, using a dichotomic approach. At the end of the processing, the
smallest possible TS file which reproduces the crash is extracted.

The script test.sh illustrates the usage.
