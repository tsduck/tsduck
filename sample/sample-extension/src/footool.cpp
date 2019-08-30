// Sample TSDuck extension.
// This is a sample tool main code.

#include "tsduck.h"
#include "foo.h"
TS_MAIN(MainCode);

int MainCode(int argc, char *argv[])
{
    std::cout << "This is a sample tool using extension 'foo' over TSDuck version " << ts::GetVersion() << std::endl;
    return EXIT_SUCCESS;
}
