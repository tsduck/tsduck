enable_testing()

file(GLOB utest_SOURCES LIST_DIRECTORIES true 
    "${CMAKE_CURRENT_SOURCE_DIR}/*\.cpp")
if(NOT BUILD_SHARED_LIBS)
    list(REMOVE_ITEM utest_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/utestPluginRepository.cpp")
endif()

add_executable(utest ${utest_SOURCES})
target_link_libraries(utest ${tsduck_libs})

set(tsduck_BINARY_DIR ${CMAKE_BINARY_DIR})
set(TSPLUGIN_BUILT_DIR ${tsduck_BINARY_DIR}/src/tsplugins/lib/tsduck)
list(APPEND TSDUCK_LD_LIBS_PATH ${tsduck_BINARY_DIR})
list(APPEND TSDUCK_LD_LIBS_PATH ${CMAKE_BINARY_DIR})
list(APPEND TSDUCK_LD_LIBS_PATH ${TSPLUGIN_BUILT_DIR})
set(LD_LIBS_PATH ${TSDUCK_LD_LIBS_PATH})
list(APPEND LD_LIBS_PATH ${CMAKE_BINARY_DIR})
list(JOIN LD_LIBS_PATH ":" LD_LIBS_PATH)

add_test(
    NAME "utest"
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/utest
)
set_tests_properties("utest" PROPERTIES
    ENVIRONMENT "LD_LIBRARY_PATH=${LD_LIBS_PATH}:$ENV{LD_LIBRARY_PATH};TSPLUGINS_PATH=${TSPLUGIN_BUILT_DIR};PATH=${tsduck_BINARY_DIR}/src/tstools:$ENV{PATH};TS_CURL_RETRY=RETRY=5,INTERVAL=100,HOST=tsduck.io"
)
