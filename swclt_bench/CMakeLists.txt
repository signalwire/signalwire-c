add_executable(swclt_bench main.c)
target_link_libraries(swclt_bench signalwire_client)
target_include_directories(swclt_bench PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set_target_properties(swclt_bench PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

configure_file(cfg/swclt_bench.cfg ${CMAKE_BINARY_DIR}/swclt_bench.cfg COPYONLY)
