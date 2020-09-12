// Sample TSDuck extension.
// This is a sample tool main code.

#include "foo.h"
TS_MAIN(MainCode);


//----------------------------------------------------------------------------
//  Command line options
//----------------------------------------------------------------------------

namespace foo {
    class FooToolOptions: public ts::Args
    {
        TS_NOBUILD_NOCOPY(FooToolOptions);
    public:
        FooToolOptions(int argc, char *argv[]);
        virtual ~FooToolOptions();

        ts::DuckContext   duck;     // TSDuck execution context.
        bool              all;      // A useless option.
        ts::UStringVector infiles;  // Some useless input file names.
    };
}

foo::FooToolOptions::FooToolOptions(int argc, char *argv[]) :
    Args(u"A sample useless utility in the Foo extension to TSDuck", u"[options] [filename ...]"),
    duck(this),
    all(false),
    infiles()
{
    option(u"", 0, STRING, 0, ts::Args::UNLIMITED_COUNT);
    help(u"", u"Input files (standard input if omitted).");

    option(u"all", 'a');
    help(u"all", u"Does not mean anything, this is just a sample option.");

    analyze(argc, argv);

    getValues(infiles, u"");
    all = present(u"all");

    exitOnError();
}

foo::FooToolOptions::~FooToolOptions()
{
}


//----------------------------------------------------------------------------
//  Program entry point
//----------------------------------------------------------------------------

int MainCode(int argc, char *argv[])
{
    foo::FooToolOptions opt(argc, argv);

    opt.info(u"This is a sample tool using extension 'foo' over TSDuck version %s", {ts::VersionInfo::GetVersion()});
    opt.verbose(u"Option --all is %s, number of input files: %d", {opt.all, opt.infiles.size()});

    return EXIT_SUCCESS;
}
