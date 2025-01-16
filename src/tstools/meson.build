
TOOLS = [
    'tsdektec.cpp',
    'tsxml.cpp',
#    'tssmartcard.cpp', // no support for PCSC
    'tscharset.cpp',
    'tscrc32.cpp',
    'tsecmg.cpp',
    'tsresync.cpp',
    'tsftrunc.cpp',
    'tscmp.cpp',
    'tspacketize.cpp',
    'tsemmg.cpp',
    'tsdump.cpp',
    'tslatencymonitor.cpp',
    'tspcap.cpp',
    'tstabdump.cpp',
    'tshides.cpp',
    'tseit.cpp',
    'tspcontrol.cpp',
    'tsbitrate.cpp',
    'tsvatek.cpp',
    'tsp.cpp',
    'tsscan.cpp',
    'tspsi.cpp',
    'tsfuzz.cpp',
    'tsswitch.cpp',
    'tstables.cpp',
    'tsanalyze.cpp',
    'tsstuff.cpp',
    'tsfixcc.cpp',
    'tsterinfo.cpp',
    'tstestecmg.cpp',
    'tstabcomp.cpp',
    'tsgenecm.cpp',
    'tsfclean.cpp',
    'tslsdvb.cpp',
    'tsdate.cpp',
    'tsversion.cpp'
]

foreach tool : TOOLS
    if tool == 'tsp.cpp'
        executable(tool.split('.cpp')[0], tool, dependencies: [tsduck_dep, TSPLUGINS])
    else
        executable(tool.split('.cpp')[0], tool, dependencies: [tsduck_dep])
    endif
endforeach