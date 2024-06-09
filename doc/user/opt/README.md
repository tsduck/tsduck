# Common groups of options

This directory contains documentation files for individual options or groups
of options which are used in several commands or plugins.

These files are typically included from other `.adoc` files.

Groups of options are options which are defined and loaded in a given C++ class
which is used in several commands and/or plugins. There is a corresponding `.adoc`
file for the documentation of these options.

- `group-*.adoc`: files containing a complete group of options, including a
  section title with role `[.usage]'.
- `opt-*.adoc`: files containing one single option, without section title.
- `optdoc-*.adoc`: files containing one single `optdoc` paragraph.
- `table-*.adoc`: files containing a table.
