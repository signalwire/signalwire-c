cmake_minimum_required(VERSION 3.6.2)

option(SWCLT_DEBUG_JSON "Enables json frame logs to stdout" OFF)

include(cmake/cotire.cmake)

if (WIN32)
	# Setup hunter gate to auto grab our 3rd party dependencies
	# (may be included by our super project so check HUNTER_WIKI)
	if (NOT HUNTER_WIKI)
		include(cmake/HunterGate.cmake)
		HunterGate(
			URL "https://github.com/ruslo/hunter/archive/v0.20.39.tar.gz"
			SHA1 "b49c4b58e17c1473e822f8b21fcde92fee25791d"
		)
	endif()
endif()

# Declare our project, libks
project(Signalwire-Client-C VERSION 1.0.0 LANGUAGES C)
message("Signalwire-Client-C ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# Set package version
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH "0")

include(cmake/FindLibKS.cmake)
include(${LIBKS_CMAKE_DIR}/ksutil.cmake)
ksutil_setup_platform()

# Find stuff we need for packaging on UNIX
if(KS_PLAT_LIN)
	find_program(LSB_RELEASE_CMD lsb_release)
	find_package(Git)
	find_program(GZIP_CMD gzip)
	find_program(DATE_CMD date)
endif()

# Fixes build on older gcc, Debian Jessie
if(KS_PLAT_LIN)
	set(CMAKE_C_STANDARD 99)
endif()

# Set package name
string(TOLOWER ${CMAKE_PROJECT_NAME} PACKAGE_NAME)

# Set package contact
set(CPACK_PACKAGE_CONTACT "support@signalwire.com")

# Detect architecture
if(NOT CPACK_SYSTEM_NAME)
	set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_PROCESSOR})
	# sanity check
	if(CPACK_SYSTEM_NAME STREQUAL x86_64)
		set(CPACK_SYSTEM_NAME amd64)
	endif()
endif()

# Get UNIX type
if(KS_PLAT_LIN)
	find_file(DEBIAN_FOUND debian_version debconf.conf
		PATHS /etc
	)
	if(DEBIAN_FOUND)
		set(CMAKE_OS_NAME "Debian" CACHE STRING "Operating system name" FORCE)
	endif()
endif()

# Get distro codename
if (KS_PLAT_LIN AND NOT DISTRO_CODENAME)
	if(NOT LSB_RELEASE_CMD)
		message(WARNING "DEB Generator: Can't find lsb_release in your path. Setting DISTRO_CODENAME to unknown.")
		set(DISTRO_CODENAME unknown)
	else()
		execute_process(COMMAND ${LSB_RELEASE_CMD} -cs
			OUTPUT_VARIABLE DISTRO_CODENAME
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif()
endif()

# Debian packaging
if("${CMAKE_OS_NAME}" STREQUAL "Debian")

	# https://gitlab.kitware.com/cmake/cmake/issues/16517
	set(CPACK_COMPONENTS_ALL runtime)
	set(CPACK_DEB_COMPONENT_INSTALL "ON")

	# Set package name
	set(CPACK_DEBIAN_PACKAGE_NAME "signalwire-client-c")

 	# Abuse Cmake component name
 	set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME ${CPACK_DEBIAN_PACKAGE_NAME})

	# Find stuff we need for packaging on Debian
	find_program(DPKG_CMD dpkg)

	# Set package type to build
	set(CPACK_GENERATOR DEB)

	# Set package maintainer
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})

	# Manual package dependencies
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

	# Detect package dependencies
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
	set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
	set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")

	# Set package description
	set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "SignalWire Client C")

	# Set package section
	set(CPACK_DEBIAN_PACKAGE_SECTION "devel")

	# Set package priority
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

	# Set package architecture
	set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CPACK_SYSTEM_NAME})

	# Enforce Debian policy permission rules
	set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

	# Set a Debian compliant changelog footer
	if(DATE_CMD)
		execute_process(COMMAND ${DATE_CMD} -R OUTPUT_VARIABLE RFC2822_TIMESTAMP)
		# Two spaces between maintainer and timestamp is NOT a mistake
		set(CHANGELOG_FOOTER " -- ${CPACK_DEBIAN_PACKAGE_MAINTAINER}  ${RFC2822_TIMESTAMP}")
	endif()

	# Set version release from environment variable
	if (NOT "$ENV{PACKAGE_RELEASE}" STREQUAL "")
		set(CPACK_DEBIAN_PACKAGE_RELEASE "$ENV{PACKAGE_RELEASE}")
	else()
		if(DATE_CMD)
			execute_process(COMMAND ${DATE_CMD} +%Y%m%d OUTPUT_VARIABLE DATE_YMD)
			set(CPACK_DEBIAN_PACKAGE_RELEASE ${DATE_YMD})
		else()
			set(CPACK_DEBIAN_PACKAGE_RELEASE "1")
		endif()
	endif()

	# Set package version
	set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}~${DISTRO_CODENAME})

	# Set debian file name format
	set(CPACK_DEBIAN_FILE_NAME "${PACKAGE_NAME}_${PROJECT_VERSION}-${CPACK_DEBIAN_PACKAGE_RELEASE}_${DISTRO_CODENAME}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")

	# Set a Debian compliant changelog header
	set(CHANGELOG_HEADER "${PACKAGE_NAME} (${CPACK_DEBIAN_PACKAGE_VERSION}) ${DISTRO_CODENAME}\; urgency=${CPACK_DEBIAN_PACKAGE_PRIORITY}")

	# Generate a Debian compliant changelog
	if(GIT_FOUND AND GZIP_CMD AND DATE_CMD)
		# Get git log info
		execute_process(COMMAND ${GIT_EXECUTABLE} log --no-merges --pretty=format:"%n  [%an]%n   * %s" --since="last month"
			OUTPUT_VARIABLE CHANGELOG
			WORKING_DIRECTORY .
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		string(REPLACE "\"" "" CHANGELOG ${CHANGELOG})

		# Create changelog
		file(WRITE changelog.Debian ${CHANGELOG_HEADER}\n${CHANGELOG}\n\n${CHANGELOG_FOOTER})
		execute_process(COMMAND ${GZIP_CMD} -f -9 -n changelog.Debian)

		# Install changelog
		install(FILES "${CMAKE_BINARY_DIR}/changelog.Debian.gz" COMPONENT "runtime" DESTINATION "share/doc/${CPACK_DEBIAN_PACKAGE_NAME}")

		# Delete changelog related variables
		unset(CHANGELOG_HEADER)
		unset(CHANGELOG_FOOTER)
		unset(RFC2822_TIMESTAMP)
	else()
		message(WARNING "DEB Generator: Can't find git and/or gzip and/or date in your path. DEB packages will be missing changelog.Debian.gz.")
	endif()
endif()

# Enable packaging module
include(CPack)

# This setting defers the rpath generation to be set at install time, allowing
# us to run the apps from the build dir without installing (come install time
# the binary is re-linked with an rpath matching that of the install prefix)
set(SKIP_BUILD_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Use the link path for the rpath
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

file(GLOB_RECURSE SWCltSrc [LIST_DIRECTORIES false]
  ${CMAKE_CURRENT_LIST_DIR}/src/*.c
)

file(GLOB_RECURSE SWCltInc [LIST_DIRECTORIES false]
  ${CMAKE_CURRENT_LIST_DIR}/inc/*.h
)

if (KS_PLAT_WIN)
	source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SWCltSrc})
	source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${SWCltInc})
endif()

add_library(signalwire_client SHARED ${SWCltSrc} ${SWCltInc})
set_target_properties(signalwire_client PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(signalwire_client PROPERTIES DEBUG_POSTFIX "")

# Define our exports symbol to key any definitions to toggle the visibility type
set_target_properties(signalwire_client PROPERTIES DEFINE_SYMBOL SWCLT_EXPORTS)

# Setup blade core definnitions
target_compile_definitions(signalwire_client PUBLIC
  -DSWCLT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  -DSWCLT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  -DSWCLT_VERSION_REVISION=${PROJECT_VERSION_PATCH}
)

if (KS_PLAT_WIN)
	target_compile_definitions(signalwire_client PUBLIC)
endif()

if (SWCLT_DEBUG_JSON)
	target_compile_definitions(signalwire_client PUBLIC
	  -DSWCLT_DEBUG_JSON=1
	)
endif()

# Copy our ca dir to the binary dir so we can run these things in the build env
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/ca)
	file(COPY ${CMAKE_CURRENT_LIST_DIR}/ca DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Setup ks core dependent linkages
if (NOT WIN32)
		target_link_libraries(signalwire_client PUBLIC ks)
else()
		target_link_libraries(
			signalwire_client PUBLIC
			${LIBKS_LIBRARY_PATH}/ks.lib
			OpenSSL::SSL
			OpenSSL::Crypto
		)
endif()

# Include key paths
target_include_directories(
  signalwire_client PUBLIC
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/libks>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

if (WIN32)
# Include key paths
		target_include_directories(
			signalwire_client PUBLIC
			$<BUILD_INTERFACE:;${LIBKS_INCLUDE_DIRS}/src/include>
			$<BUILD_INTERFACE:;${OPENSSL_INCLUDE_DIR}>
		)
endif()

target_include_directories(
	signalwire_client PRIVATE
)

# Include the bin dir for config discovery
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")

if (NOT KS_PLAT_WIN)
	# Set install targets
	install(TARGETS signalwire_client COMPONENT "runtime" EXPORT SignalWireClientConfig DESTINATION lib)
	install(DIRECTORY inc/signalwire-client-c COMPONENT "runtime" DESTINATION include PATTERN internal EXCLUDE)
	install(FILES ${PROJECT_BINARY_DIR}/signalwire_client.pc COMPONENT "runtime" DESTINATION lib/pkgconfig)
	install(EXPORT SignalWireClientConfig COMPONENT "runtime" DESTINATION include/signalwire-client-c/cmake)
	install(FILES ${PROJECT_BINARY_DIR}/copyright COMPONENT "runtime" DESTINATION share/doc/signalwire-client-c)

	# Set uninstall target
	if(NOT TARGET uninstall)
		configure_file(
			"${CMAKE_CURRENT_SOURCE_DIR}/uninstall.cmake.in"
			"${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
			IMMEDIATE @ONLY)

		add_custom_target(uninstall
			COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)
	endif()

	# Create a package config for registration with the system
	# prefix=@PC_PREFIX@
	# exec_prefix=@PC_EXEC_PREFIX@
	# libdir=@PC_LIB_DIR@
	# includedir=@PC_INCLUDE_DIR@
	# definitions=@PC_DEFINITIONS@
	# cmakedir=@PC_CMAKE_DIR@
	# Name: @PACKAGE_NAME@
	# Version: @PACKAGE_VERSION@
	set(PC_PREFIX ${CMAKE_INSTALL_PREFIX})
	get_property(PC_DEFINITIONS TARGET signalwire_client PROPERTY INTERFACE_COMPILE_DEFINITIONS)
	get_property(PC_DEFINITIONS TARGET ks PROPERTY INTERFACE_COMPILE_DEFINITIONS)
	set(PACKAGE_NAME signalwire_client)
	set(PACKAGE_VERSION ${PROJECT_VERSION})
	configure_file("${CMAKE_CURRENT_LIST_DIR}/signalwire_client.pc.in" "${PROJECT_BINARY_DIR}/signalwire_client.pc" @ONLY)
endif()

cotire(signalwire_client)

add_subdirectory(swclt_bench)

enable_testing()
add_subdirectory(swclt_test)
