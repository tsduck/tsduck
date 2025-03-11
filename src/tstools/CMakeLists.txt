add_executable(tsp tsp.cpp)
target_link_libraries(tsp PRIVATE ${tsduck_libs})
target_compile_definitions(tsp PRIVATE TS_WINVER_FILETYPE=VFT_APP)

add_executable(tsxml tsxml.cpp)
target_compile_definitions(tsxml PRIVATE TS_WINVER_FILETYPE=VFT_APP)
target_link_libraries(tsxml PRIVATE ${tsduck_libs})

add_executable(tsversion tsversion.cpp)
target_compile_definitions(tsversion PRIVATE TS_WINVER_FILETYPE=VFT_APP)
target_link_libraries(tsversion PRIVATE ${tsduck_libs})

add_executable(tslatencymonitor tslatencymonitor.cpp)
target_compile_definitions(tslatencymonitor PRIVATE TS_WINVER_FILETYPE=VFT_APP)
target_link_libraries(tslatencymonitor PRIVATE ${tsduck_libs})

install(TARGETS tsxml tsp tsversion tslatencymonitor
    COMPONENT tsduck
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
