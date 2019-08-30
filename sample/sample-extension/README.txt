This directory contains a complete sample third-party TSDuck extension.

This extension, imaginatively named "foo", illustrates all possibilities of
TSDuck extensions:

- Handling of a third-party table named "Foo Table" or FOOT. The table can be
  manipulated in XML, analyzed and displayed with the standard TSDuck tools.

- Handling of a third-party descriptor named foo_descriptor. The descriptor can
  be manipulated in XML, analyzed and displayed with the standard TSDuck tools.

- Handling a third-party Conditional Access System named "Foo CAS". The ECM's,
  EMM's and private parts of the CA_descriptor are correctly analyzed and
  displayed with the standard TSDuck tools.

- Some additional plugins for "tsp". Here there is one which manipulates the
  Foo Table on the fly.

- Some additional utilities. Here, there is one which does not do much (just
  to illustrate how to do that).

- Provide scripts to build standard installers (.exe on Windows, .rpm and .deb
  on Linux). The installers install the extension on top of a matching version
  of TSDuck.
