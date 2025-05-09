# DSM-CC Definitions

The global framework for DSM-CC (Digital Storage Media Command and Control) is defined
by MPEG in ISO/IEC 13818-6. The DSM-CC sections and tables, as well as specific descriptors,
are defined by ISO/IEC 13818-6. Their implementations are defined in `libtsduck/dtv/tables/mpeg`.

Additional descriptors for DSM-CC tables are defined by DVB in ETSI EN 301 192. Their
implementations are defined in `libtsduck/dtv/tables/dvb`.

Additional descriptors for DSM-CC tables are defined by ATSC in A/90. Their implementations
are defined in `libtsduck/dtv/tables/atsc`.

Other DSM-CC data structures (other than tables and descriptors) are implemented in this
directory (`libtsduck/dtv/dsmcc`).
