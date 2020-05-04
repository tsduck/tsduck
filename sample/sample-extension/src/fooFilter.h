// Sample TSDuck extension.
// Definition of a section filter for ts::TablesLogger.
//
// This class adds the option --foo-id to commands "tstables" and "tspsi",
// as well as the corresponding plugins "tables" and "psi". The new option
// allows the filtering of sections based on the "foo_id" in a Foo Table
// (FOOT) and ECM's or EMM's from FooCAS.

#pragma once
#include "foo.h"

namespace foo {

    class FOODLL FooFilter: public ts::TablesLoggerFilterInterface
    {
        TS_NOCOPY(FooFilter);
    public:
        // Default constructor.
        FooFilter();

        // Implementation of TablesLoggerFilterInterface.
        virtual void defineFilterOptions(ts::Args& args) const override;
        virtual bool loadFilterOptions(ts::DuckContext& duck, ts::Args& args, ts::PIDSet& initial_pids) override;
        virtual bool filterSection(ts::DuckContext& duck, const ts::Section& section, uint16_t cas, ts::PIDSet& more_pids) override;

    private:
        bool               _negate_id;  // Negate the foo_id filter (exclude selected ids).
        std::set<uint16_t> _ids;        // Foo_id values to filter.
    };
}
