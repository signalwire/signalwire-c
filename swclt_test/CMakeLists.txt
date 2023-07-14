# Our core file list
file(GLOB swclt_test_deps [LIST_DIRECTORIES false]
	${CMAKE_CURRENT_LIST_DIR}/*.h
	${CMAKE_CURRENT_LIST_DIR}/*.c
	${CMAKE_CURRENT_LIST_DIR}/cases/*.c
	${CMAKE_CURRENT_LIST_DIR}/cases/*.h
	${CMAKE_CURRENT_LIST_DIR}/util/*.h
)

if (KS_PLAT_WIN)
	source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${swclt_test_deps})
endif()

add_executable(
	swclt_test
	${swclt_test_deps}
)

# Link to signal-wire-client/catch/LibPal
target_link_libraries(swclt_test signalwire_client2)

# Define our exports symbol to key any definitions to toggle the visibility type
set_target_properties(signalwire_client2 PROPERTIES DEFINE_SYMBOL SWCLT_EXPORTS)

# Register our tests with cmake
add_test(swclt_test swclt_test)

# Include our root
target_include_directories(
	swclt_test
	PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
)

# Copy the config files to the build dir
# configure_file(cfg/swclt_test.cfg ${CMAKE_BINARY_DIR}/swclt_test.cfg COPYONLY)

# When debugging on windows, the cwd will be the binary dir (where the config files are)
set_target_properties(swclt_test PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set(COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES 1)

set_target_properties(swclt_test PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
set_target_properties(swclt_test PROPERTIES COTIRE_ENABLE_PRECOMPILED_HEADER TRUE)
set_target_properties(swclt_test PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT "swclt_test.h")
cotire(swclt_test)
