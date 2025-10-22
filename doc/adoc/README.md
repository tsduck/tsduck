## Common configuration for Asciidoctor

- `attributes.adoc`: All common attributes for TSDuck documentation.
   Must be included in the header of each document.

- `tsduck.css`: CSS style sheet for HTML output.

- `tsduck-theme.yml`: PDF theme for PDF output.

- `macros.rb`: A few basic Asciidoctor macros, written in Ruby.

### Docinfo

The docinfo files for asciidoctor are built in the binary area.

- `docinfo.html`: Main docinfo file, define some styles which are used in the docinfo footer.

- `docinfo-footer.in.html`: Input skeleton for the docinfo footer.
  We add the `tocbot` script before usage by asciidoctor (see below).

### HTML table of contents

Using `asciidoctor`, the table of contents in the sidebar of the HTML is flat.
All entries are listed, sequentially. In the case of the TSDuck User Guide,
the table contains hundredths of lines and becomes difficult to use.

This directory contains scripts to make it expandable.

The JavaScript `tocbot.js` (and its stripped version `tocbot.min.js`) are
copied from https://github.com/tscanlin/tocbot

This script is delivered under the MIT License (_"Copyright (c) 2016 Tim Scanlin"_).
See the [license text here](https://github.com/tscanlin/tocbot/blob/master/LICENSE).

However, this script is also inspired from [tocify](http://gregfranko.com/jquery.tocify.js/),
another JavaScript processing of HTML tables of contents. It seems that many scripts are
copied on each other, making the result a global collaborative work.

The version of `tocbot` which is used here is version 4.28.2:

- https://github.com/tscanlin/tocbot/releases/download/v4.28.2/tocbot.min.js
