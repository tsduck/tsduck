# Developing TSDuck extensions   {#extensions}

Applications or TSP plugins can be developed on their own.
But it is also possible to develop fully integrated _extensions_ to TSDuck.

An extension not only adds new plugins and commands, it can also augment the
features of standard TSDuck commands and plugins. An extension can also be
packaged as a binary installer which can be deployed on top of an existing
installation of TSDuck.

The possible features of a TSDuck extensions are:

- Handling third-party tables and descriptors. The new tables and descriptors
  can be manipulated in XML, analyzed and displayed with the standard TSDuck tools.

- Handling third-party Conditional Access Systems, based on a range of _CA_system_id_ values.
  The ECM's, EMM's and private parts of the _CA_descriptor_ are correctly analyzed and
  displayed with the standard TSDuck tools.

- Adding filtering capabilities based on specific or private conditions on sections
  for command `tstables` and plugin `tables`.

- Additional plugins for `tsp`.

- Additional command-line utilities.

A [complete example](https://github.com/tsduck/tsduck/tree/master/sample/sample-extension)
of a TSDuck extension is provided in the TSDuck source tree. This example also provides
scripts to build standard installers (`.exe` on Windows, `.rpm` and `.deb` on Linux).
The generated packages install the extension on top of a matching version of TSDuck.

# Files in an extension  {#extfiles}

A TSDuck extension typically contains the following types of files:

- Additional utilities. They are executable files without predefined naming. They are
  installed in the same directory as the TSDuck commands.

- Additional `tsp` plugins. They are dynamic libraries named `tsplugin_XXX.so`, `.dylib` or `.dll`.
  The plugins are loaded by `tsp` when invoked by their names `XXX`.

- Extension dynamic libraries named `tslibext_XXX.so`, `.dylib` or `.dll`. All shareable libraries
  named `tslibext_XXX` in the same directory as the TSDuck binaries or in the path
  `TSPLUGINS_PATH` are automatically loaded when any TSDuck command is invoked
  (in fact any time the TSDuck library `tsduck.dll` or `libtsduck.so` or `.dylib` is used).
  Such libraries typically install hooks into the core of TSDuck to handle third-party
  signalization.

- XML files describing the XML models for third-party signalization (tables and descriptors).
  There is no mandatory naming template for those files but `tslibext_XXX.xml` is recommended.
  These XML files must be registered by the extension dynamic library (details below).

- Name files describing third party identifiers (table ids, descriptor tags, CA system id,
  stream types, etc.) These files are used by TSDuck to better identify the various entities.
  There is no mandatory naming template for those files but `tslibext_XXX.names` is recommended.
  These files must be registered by the extension dynamic library (details below).

# The extension dynamic library  {#extdll}

All shareable libraries named `tslibext_XXX.so`, `.dylib` or `.dll` are automatically loaded by any
TSDuck command or plugin. The initialization of the library is responsible for registering
various hooks which implement the additional features.

## Identification of the extension  {#extid}

This is an optional but recommended step. One C++ module inside the `tslibext_XXX`
library shall invoke macro @link TS_REGISTER_EXTENSION @endlink as illustrated below:

~~~
TS_REGISTER_EXTENSION(u"foo",                     // extension name
                      u"Sample foo extension",    // short description
                      {u"foot", u"foobar"},       // list of provided plugins for tsp
                      {u"footool", u"foocmd"});   // list of provided command-line tools
~~~

Using this declaration, the extension is identified and listed using the command `tsversion --extensions`.

Without the declaration, the extension is loaded and functional but it is not identified.

## Providing an XML model file for additional tables and descriptors  {#extxml}

To analyze input XML files containing tables, TSDuck uses an XML model to validate
the syntax of the input XML file. There is a predefined large XML file which
describes all supported tables and descriptors.

An extension may provide additional smaller XML files which describe the new tables
or descriptors. See the sample extension for more details. The XML files shall
be installed in the same directory as the rest of the extension (and TSDuck
in general).

For each additional XML file, there must be one C++ module inside the `tslibext_XXX`
library which invokes the macro @link TS_REGISTER_XML_FILE @endlink as illustrated below:

~~~
TS_REGISTER_XML_FILE(u"tslibext_foo.xml");
~~~

## Providing a names files for additional identifiers  {#extnames}

The usage rules and conventions are identical to the XML file above.
The declaration macro for each names file is @link TS_REGISTER_NAMES_FILE @endlink
as illustrated below:

~~~
TS_REGISTER_NAMES_FILE(u"tslibext_foo.names");
~~~

Here is an example, from the sample "foo" extension, which defines additional names
for a table, a descriptor and a range of _CA_system_id_.
~~~
[TableId]
0xF0 = FOOT

[DescriptorId]
0xE8 = Foo

[CASystemId]
0xF001-0xF008 = FooCAS
~~~

## Providing support for additional tables  {#exttables}

If your environment defines a third-party table which is unsupported or unknown in TSDuck,
you can implement it in your extension library.

First, define the C++ class implementing the table:
~~~
class FooTable : public ts::AbstractLongTable { ... };
~~~

In the implementation of the table, register hooks for the various features you support.
In this example, we register a C++ class for `FooTable`:
~~~
TS_REGISTER_TABLE(FooTable,                  // C++ class name
                  {0xF0},                    // table id 0xF0
                  ts::Standards::NONE,       // not defined in any standard
                  u"FOOT",                   // XML name is <FOOT>
                  FooTable::DisplaySection);
~~~
The last argument to @link TS_REGISTER_TABLE @endlink is a static method of the class which
displays the content of a section of this table type.

The XML model for the table is included in the XML file:
~~~
<?xml version="1.0" encoding="UTF-8"?>
<tsduck>
  <_tables>
    <FOOT version="uint5, default=0" current="bool, default=true" foo_id="uint16, required" name="string, optional">
      <_any in="_descriptors"/>
    </FOOT>
  </_tables>
</tsduck>
~~~
for the following binary layout:
~~~
table_id                     8 bits   = 0xF0
section_syntax_indicator     1 bit    = '1'
reserved                     3 bits
section_length              12 bits
foo_id                      16 bits
reserved                     2 bits
version_number               5 bits
current_next_indicator       1 bit
section_number               8 bits
last_section_number          8 bits
name_length                  8 bits
for(i=0;i<N;i++){
    name_char                8 bits
}
reserved_future_use          4 bits
descriptors_length          12 bits
for (i=0;i<N;i++){
    descriptor()
}
CRC_32
~~~

## Providing support for additional descriptors  {#extdescs}

Similarly, it is possible to implement a third-party descriptor as follow:
~~~
class FooDescriptor : public ts::AbstractDescriptor { ... };
~~~

In the implementation of the descriptor, we register hooks for the various features.
Since this is a non-DVB descriptor with descriptor tag `0xE8`, greater than `0x80`, we must
set the private data specifier to zero in the ts::EDID ("extended descriptor id").
~~~
TS_REGISTER_DESCRIPTOR(FooDescriptor,                 // C++ class name
                       ts::EDID::Private(0xE8, 0),    // "extended" descriptor id
                       u"foo_descriptor",             // XML name is <foo_descriptor>
                       FooDescriptor::DisplayDescriptor);
~~~
The last argument to @link TS_REGISTER_DESCRIPTOR @endlink is a static method of the class which
displays the content of a descriptor.

The XML model for the descriptor is included in the XML file:
~~~
<?xml version="1.0" encoding="UTF-8"?>
<tsduck>
  <_descriptors>
    <foo_descriptor name="string, required"/>
  </_descriptors>
</tsduck>
~~~
for the following binary layout:
~~~
descriptor_tag           8 bits = 0xE8
descriptor_length        8 bits
for(i=0;i<N;i++) {
    name_char            8 bits
}
~~~

## Implementing advanced section filtering capabilities  {#extfilter}

The command `tstables` (and its plugin counterpart `tables`) can process
vast amounts of tables. To extract specific tables or sections, the command
provides filtering options such as `--pid`, `--tid` or `--tid-ext`.

For specific sections, it is possible to define additional filtering options
to the `tstables` command.

The extension library shall provide a C++ class implementing ts::TablesLoggerFilterInterface.
The sample "foo" extension provide an option `--foo-id` which selects instances of
`FooTable` containing specific values for some "foo_id" field.
~~~
class FooFilter: public ts::TablesLoggerFilterInterface { ... };
~~~

See the documentation of ts::TablesLoggerFilterInterface for more details.

In the implementation of the class, we register it as a section filter for `tstables`:
~~~
TS_REGISTER_SECTION_FILTER(FooFilter);
~~~

## Providing support for additional Conditional Access Systems  {#extcas}

If you work with a specific Conditional Access System, you probably manipulate
confidential information that cannot be published in an open-source tool such
as TSDuck. The solution is to develop a private closed-source extension.

In the extension library, you may register functions to display the structure
of the ECM's, EMM's or private part of the _CA_descriptor_. The registration
is based on a range of _CA_system_id_ (here the constants `CASID_FOO_MIN`
and `CASID_FOO_MAX`).

~~~
// Display a FooCAS ECM on the output stream.
// Compatible with ts::DisplaySectionFunction profile.
void DisplayFooCASECM(ts::TablesDisplay& display, const ts::Section& section, int indent);

// Display a FooCAS EMM on the output stream.
// Compatible with ts::DisplaySectionFunction profile.
void DisplayFooCASEMM(ts::TablesDisplay& display, const ts::Section& section, int indent);

// Display the payload of a FooCAS ECM on the output stream as a one-line "log" message.
// Compatible with ts::LogSectionFunction profile.
ts::UString LogFooCASECM(const ts::Section& section, size_t max_bytes);

// Display the payload of a FooCAS EMM on the output stream as a one-line "log" message.
// Compatible with ts::LogSectionFunction profile.
ts::UString LogFooCASEMM(const ts::Section& section, size_t max_bytes);

// Display the private part of a FooCAS CA_descriptor on the output stream.
// Compatible with ts::DisplayCADescriptorFunction profile.
void DisplayFooCASCADescriptor(ts::TablesDisplay& display, const uint8_t* data, size_t size, int indent, ts::TID tid);
~~~
See the documentation for ts::DisplaySectionFunction, ts::LogSectionFunction and ts::DisplayCADescriptorFunction.

To register the display handlers in TSDuck:
~~~
TS_REGISTER_SECTION({ts::TID_ECM_80, ts::TID_ECM_81},
                    ts::Standards::NONE,  // not defined in any standard
                    DisplayFooCASECM,     // display function
                    LogFooCASECM,         // one-line log function
                    {},                   // no predefined PID
                    CASID_FOO_MIN,        // range of CA_system_id
                    CASID_FOO_MAX);

TS_REGISTER_SECTION(ts::Range<ts::TID>(ts::TID_EMM_FIRST, ts::TID_EMM_LAST),
                    ts::Standards::NONE,  // not defined in any standard
                    DisplayFooCASEMM,     // display function
                    LogFooCASEMM,         // one-line log function
                    {},                   // no predefined PID
                    CASID_FOO_MIN,        // range of CA_system_id
                    CASID_FOO_MAX);

TS_REGISTER_CA_DESCRIPTOR(DisplayFooCASCADescriptor, CASID_FOO_MIN, CASID_FOO_MAX);
~~~

# Building cross-platform binary installers for an extension  {#extinstaller}

See the [sample "foo" extension](https://github.com/tsduck/tsduck/tree/master/sample/sample-extension)
in the TSDuck source tree.

Scripts are provided to build `.exe` installers on Windows, `.rpm` and `.deb` packages on Linux.
