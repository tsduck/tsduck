//----------------------------------------------------------------------------
//
// TSDuck sample application using ts::SignalizationDemux on a TS file.
//
//----------------------------------------------------------------------------

#include "tsduck.h"

// Enforce COM and network init on Windows, transparent elsewhere.
TS_MAIN(MainCode);

//----------------------------------------------------------------------------
// Command line options: a list of TS files.
//----------------------------------------------------------------------------

class Options: public ts::Args
{
    TS_NOBUILD_NOCOPY(Options);
public:
    Options(int argc, char *argv[]);
    ts::UStringVector input_files {};
};

Options::Options(int argc, char *argv[]) :
    Args(u"Sample usage of SignalizationDemux", u"[input-file ...]")
{
    option(u"", 0, FILENAME, 0, UNLIMITED_COUNT);
    help(u"", u"List of input TS files.");

    analyze(argc, argv);

    getValues(input_files, u"");
    exitOnError();
}

//----------------------------------------------------------------------------
// A class to analyze a TS file using a SignalizationDemux.
//----------------------------------------------------------------------------

class Analyzer: private ts::SignalizationHandlerInterface
{
public:
    // Analyze a TS file.
    void analyze(const ts::UString& filename);

private:
    // Implementation of some methods from SignalizationHandlerInterface.
    // They don't do much, just display a message for demo.
    virtual void handlePAT(const ts::PAT&, ts::PID) override;
    virtual void handlePMT(const ts::PMT&, ts::PID) override;
    virtual void handleNIT(const ts::NIT&, ts::PID) override;
    virtual void handleSDT(const ts::SDT&, ts::PID) override;
    virtual void handleVCT(const ts::VCT&, ts::PID) override;
    virtual void handleService(uint16_t ts_id, const ts::Service&, const ts::PMT&, bool removed) override;

private:
    ts::DuckContext _duck {};
    ts::SignalizationDemux _demux {_duck, this};
};

// Analyze a TS file.
void Analyzer::analyze(const ts::UString& filename)
{
    std::cout << "==== Analyzing " << filename << std::endl;

    ts::TSFile file;
    ts::TSPacketVector pkt(1000);
    ts::TSPacketMetadataVector meta(pkt.size());
    size_t count = 0;

    // Filter everything.
    _demux.reset();
    _demux.addFullFilters();

    if (file.open(filename, ts::TSFile::READ, CERR)) {
        while ((count = file.readPackets(&pkt[0], &meta[0], pkt.size(), CERR)) > 0) {
            for (size_t i = 0; i < count; ++i) {
                _demux.feedPacket(pkt[i]);
            }
        }
        file.close(CERR);
    }
}

void Analyzer::handlePAT(const ts::PAT& pat, ts::PID pid)
{
    std::cout << "-- Got PAT, " << pat.pmts.size() << " services" << std::endl;
}

void Analyzer::handlePMT(const ts::PMT& pmt, ts::PID pid)
{
    std::cout << "-- Got PMT, service id " << pmt.service_id << ", " << pmt.streams.size() << " components" << std::endl;
}

void Analyzer::handleNIT(const ts::NIT& nit, ts::PID pid)
{
    std::cout << "-- Got " << ts::TIDName(_duck, nit.tableId())
              << ", network id " << nit.network_id << ", " << nit.transports.size() << " TS" << std::endl;
}

void Analyzer::handleSDT(const ts::SDT& sdt, ts::PID pid)
{
    std::cout << "-- Got " << ts::TIDName(_duck, sdt.tableId())
              << ", TS id " << sdt.ts_id << ", " << sdt.services.size() << " services" << std::endl;
}

void Analyzer::handleVCT(const ts::VCT& vct, ts::PID pid)
{
    std::cout << "-- Got " << ts::TIDName(_duck, vct.tableId()) << ", " << vct.channels.size() << " channels" << std::endl;
}

void Analyzer::handleService(uint16_t ts_id, const ts::Service& service, const ts::PMT& pmt, bool removed)
{
    std::cout << "-- " << (removed ? "Removed" : "Got") << " service " << service;
    if (pmt.isValid()) {
        std::cout << ", " << pmt.streams.size() << " components";
    }
    else {
        std::cout << ", no PMT";
    }
    std::cout << std::endl;
}

//----------------------------------------------------------------------------
// Main application.
//----------------------------------------------------------------------------

int MainCode(int argc, char* argv[])
{
    Options opt(argc, argv);
    Analyzer anl;
    for (const auto& name : opt.input_files) {
        anl.analyze(name);
    }
    return EXIT_SUCCESS;
}
