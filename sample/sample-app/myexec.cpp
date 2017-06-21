#include "tsduck.h"

int main(int argc, char* argv[])
{
    // Safe pointer on a PSI/SI table.
    ts::SafePtr<ts::NIT, ts::Mutex> ptr(new ts::NIT);

    // Format a string.
    std::cout << ts::Format("format, i=%d, s=%s", 12, "abc") << std::endl
              << "TSDuck version: " << ts::GetVersion() << std::endl
              << "Built: " << ts::GetVersion(ts::VERSION_DATE) << std::endl;

    return EXIT_SUCCESS;
}
