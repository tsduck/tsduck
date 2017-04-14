//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2017, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//
//  PSI/SI table generator utility
//
//----------------------------------------------------------------------------

#include "tsArgs.h"
#include "tsGenTabPlugin.h"
#include "tsApplicationSharedLibrary.h"
#include "tsOutputRedirector.h"
#include "tsNullMutex.h"
#include "tsSafePtr.h"
#include "tsSysUtils.h"

using namespace ts;


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

struct Options: public Args
{
    Options (int argc, char *argv[]);

    std::string  plugin_name;
    StringVector plugin_options;
    std::string  output_file;
    std::string  binary_file;
    bool         list_plugins;
};

Options::Options (int argc, char *argv[]) :
    Args ("PSI/SI table generator using plugins.", "[options] plugin-name [plugin-options ...]", "", GATHER_PARAMETERS)
{
    option ("",              0,  STRING, 0, UNLIMITED_COUNT);
    option ("binary-file",  'b', STRING);
    option ("list-plugins", 'l');
    option ("output-file",  'o', STRING);

    setHelp ("Plugin name:\n"
             "  Name of the plugin to use. All tsgentab-options must be placed on the\n"
             "  command line before the plugin name. All options after the plugin name\n"
             "  are passed to the plugin.\n"
             "\n"
             "Options:\n"
             "\n"
             "  -b name\n"
             "  --binary-file name\n"
             "      Specify a file where the binary version of the table is saved.\n"
             "\n"
             "  --help\n"
             "      Display this help text.\n"
             "\n"
             "  -l\n"
             "  --list-plugins\n"
             "      List all available plugins for tsgentab. Do not generate any table.\n"
             "\n"
             "  -o name\n"
             "  --output-file name\n"
             "      Specify a file where a textual representation of the table is saved.\n"
             "      By default, if neither --binary-file nor --output-file are specified,\n"
             "      a textual representation of the table is printed on the standard output.\n"
             "\n"
             "  --version\n"
             "      Display the version number.\n");

    analyze (argc, argv);

    plugin_name = value ("");
    output_file = value ("output-file");
    binary_file = value ("binary-file");
    list_plugins = present ("list-plugins");

    for (size_t n = 1; n < count (""); ++n) {
        plugin_options.push_back (value ("", "", n));
    }
}


//----------------------------------------------------------------------------
//  Implementation of a plugin shared library
//----------------------------------------------------------------------------

class GenTabSharedLibrary: public ApplicationSharedLibrary
{
public:
    // Public fields (valid if isLoaded)
    NewGenTabPluginProfile new_plugin;

    // Constructor.
    GenTabSharedLibrary (const std::string& filename, ReportInterface& report) :
        ApplicationSharedLibrary (filename, "tsgentab_", true),
        new_plugin (0)
    {
        if (isLoaded()) {
            new_plugin = reinterpret_cast<NewGenTabPluginProfile> (getSymbol ("tsgentabNewPlugin"));
        }
        else {
            report.error (errorMessage());
        }
    }
};

// Safe pointer for GenTabSharedLibrary (not thread-safe)
typedef SafePtr <GenTabSharedLibrary, NullMutex> GenTabSharedLibraryPtr;
    

//----------------------------------------------------------------------------
// List all plugins
//----------------------------------------------------------------------------

namespace  {
    void ListPlugins (ReportInterface& report)
    {
        const std::string pattern (DirectoryName (ExecutableFile()) + PathSeparator + "tsgentab_*" + SharedLibrary::Extension);

        // Get list of shared library files
        StringVector files;
        if (!ExpandWildcard (files, pattern)) {
            report.error ("error resolving " + pattern);
            return;
        }

        // Build list of names and load shared libraries
        StringVector names (files.size());
        std::vector <GenTabSharedLibraryPtr> shlibs (files.size());
        size_t name_width = 0;

        for (size_t i = 0; i < files.size(); ++i) {
            shlibs[i] = new GenTabSharedLibrary (files[i], report);
            names[i] = shlibs[i]->moduleName();
            if (shlibs[i]->isLoaded()) {
                name_width = std::max (name_width, names[i].size());
            }
        }

        // List plugins
        for (size_t i = 0; i < files.size(); ++i) {
            if (shlibs[i]->isLoaded() && shlibs[i]->new_plugin != 0) {
                GenTabPlugin* p = shlibs[i]->new_plugin();
                std::cout << JustifyLeft (names[i] + ' ', name_width + 4, '.') << " " << p->getDescription() << std::endl;
                delete p;
            }
        }
    }
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int main (int argc, char *argv[])
{
    Options opt (argc, argv);

    // Process --list-plugins options (no table generation)
    if (opt.list_plugins) {
        ListPlugins (opt);
        return EXIT_SUCCESS;
    }

    // Check that the plugin name is present
    if (opt.plugin_name.empty()) {
        opt.error ("plugin name is missing");
        return EXIT_FAILURE;
    }

    // Load the plugin
    GenTabSharedLibrary shlib (opt.plugin_name, opt);
    if (!shlib.isLoaded()) {
        // error already reported
        return EXIT_FAILURE;
    }
    if (shlib.new_plugin == 0) {
        opt.error (shlib.fileName() + " is not a valid tsgentab plugin");
        return EXIT_FAILURE;
    }

    // Create a plugin instance
    GenTabPlugin* plugin = shlib.new_plugin();
    if (plugin == 0) {
        // Error message expected from plugin
        return EXIT_FAILURE;
    }

    // Submit the plugin arguments for analysis.
    // The process should terminate on argument error.
    plugin->setShell(opt.appName());
    plugin->analyze(opt.plugin_name, opt.plugin_options);
    assert(plugin->valid());

    // Generate the table
    AbstractTablePtr table;
    plugin->generate(table);
    if (table.isNull() || !table->isValid()) {
        // Error message expected from plugin
        return EXIT_FAILURE;
    }

    // Serialize the table
    BinaryTable bin_table;
    table->serialize(bin_table);
    if (!bin_table.isValid()) {
        opt.error("invalid table returned from " + shlib.moduleName());
        return EXIT_FAILURE;
    }

    // Save binary table if required
    if (!opt.binary_file.empty() && !bin_table.save (opt.binary_file, opt)) {
        return EXIT_FAILURE;
    }

    // Display formatted version of table
    if (opt.binary_file.empty() || !opt.output_file.empty()) {
        OutputRedirector output (opt.output_file, opt, std::cout, std::ios::out);
        bin_table.display (std::cout);
    }

    return EXIT_SUCCESS;
}
