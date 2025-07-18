cmake_minimum_required(VERSION 3.7.2)

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

# Declare our project, SignalWire-Client-C2
project(SignalWire-Client-C2 VERSION 2.0.2 LANGUAGES C)
message("SignalWire-Client-C2 ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

# Set package version
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH "0")

# Configure the version header file 
configure_file (
  "${CMAKE_CURRENT_LIST_DIR}/inc/signalwire-client-c/version.h.in"
  "${CMAKE_CURRENT_LIST_DIR}/inc/signalwire-client-c/version.h"  )

include(cmake/FindLibKS.cmake)
include(${LIBKS_CMAKE_DIR}/ksutil.cmake)
ksutil_setup_platform()

# Find stuff we need for packaging on UNIX
if(KS_PLAT_LIN)
	find_package(Git)
	find_program(GZIP_CMD gzip)
	find_program(DATE_CMD date)
endif()

# Fixes build on older gcc, Debian Jessie
if(KS_PLAT_LIN)
	set(CMAKE_C_STANDARD 99)
endif()

# Set package name
string(TOLOWER "${CMAKE_PROJECT_NAME}" PACKAGE_NAME)

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

	find_file(CENTOS_FOUND centos-release
		PATHS /etc
	)

	if(DEBIAN_FOUND)
		set(CMAKE_OS_NAME "Debian" CACHE STRING "Operating system name" FORCE)
	endif()

	if(CENTOS_FOUND)
		set(CMAKE_OS_NAME "Centos" CACHE STRING "Operating system name" FORCE)
	endif()
endif()

# Centos packaging
if("${CMAKE_OS_NAME}" STREQUAL "Centos")

	# Enable component install
	set(CPACK_RPM_COMPONENT_INSTALL ON)

	# Find stuff we need for packaging on Centos
	find_program(ARCH_CMD arch)
	find_program(RPM_CMD rpm)

	# Set package architecture
	if(ARCH_CMD)
		execute_process(COMMAND ${ARCH_CMD} OUTPUT_VARIABLE CENTOS_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
		set(CPACK_RPM_RUNTIME_PACKAGE_ARCHITECTURE ${CENTOS_ARCH})
		set(CPACK_SYSTEM_NAME ${CENTOS_ARCH})
		unset(CENTOS_ARCH)
	endif()

	# Get RPM DIST tag, rpm --eval '%{dist}'
	if(RPM_CMD)
		execute_process(COMMAND ${RPM_CMD} --eval %{dist}
			OUTPUT_VARIABLE DIST_TAG
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE)
	endif()

	# https://gitlab.kitware.com/cmake/cmake/issues/16517
	set(CPACK_COMPONENTS_ALL runtime)

	# Set package type to build
	set(CPACK_GENERATOR RPM)

	# Set RPM package name
	set(CPACK_RPM_RUNTIME_PACKAGE_NAME "signalwire-client-c2")

	# Set version release from environment variable
	if (NOT "$ENV{PACKAGE_RELEASE}" STREQUAL "")
		set(CPACK_RPM_PACKAGE_RELEASE "$ENV{PACKAGE_RELEASE}")
	else()
		if(DATE_CMD)
			execute_process(COMMAND ${DATE_CMD} +%Y%m%d OUTPUT_VARIABLE DATE_YMD)
			set(CPACK_RPM_PACKAGE_RELEASE ${DATE_YMD})
		else()
			set(CPACK_RPM_PACKAGE_RELEASE "1")
		endif()
	endif()

	# Set RPM version
	set(CPACK_RPM_PACKAGE_VERSION ${PROJECT_VERSION})

	# Set OS DIST tag
	set(CPACK_RPM_PACKAGE_RELEASE_DIST ${DIST_TAG})

	# Set RPM filename
	set(CPACK_RPM_RUNTIME_FILE_NAME "${CPACK_RPM_RUNTIME_PACKAGE_NAME}-${PROJECT_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}${DIST_TAG}.${CENTOS_ARCH}rpm")

	# Set license
	set(CPACK_RPM_PACKAGE_LICENSE "MIT")

	# Set RPM group
	set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries/C and C++")

	# Set RPM vendor
	set(CPACK_RPM_PACKAGE_VENDOR "SignalWire")

	# Set RPM URL
	set(CPACK_RPM_RUNTIME_PACKAGE_URL "https://signalwire.com/")

	# Set RPM Description
	set(CPACK_RPM_PACKAGE_DESCRIPTION "SignalWire C Client and Library Headers")

	# Set RPM package summary
	set(CPACK_RPM_PACKAGE_SUMMARY "SignalWire's C client")

	# Set auto dependency detection
	set(CPACK_RPM_PACKAGE_AUTOREQPROV "yes")

	# Create changelog
	string(TIMESTAMP NOW "%a %b %d %Y" UTC)
	FILE(WRITE ${CMAKE_CURRENT_LIST_DIR}/changelog "* ${NOW} ${CPACK_RPM_PACKAGE_VENDOR} <${CPACK_PACKAGE_CONTACT}> ${PROJECT_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}${DIST_TAG}\n")
	FILE(APPEND ${CMAKE_CURRENT_LIST_DIR}/changelog "- autogenerated RPM package\n")
	unset(NOW)
	set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_CURRENT_LIST_DIR}/changelog")

endif()

# Debian packaging
if("${CMAKE_OS_NAME}" STREQUAL "Debian")

	# Find stuff we need for packaging on Debian
	find_program(DPKG_CMD dpkg)
	find_program(LSB_RELEASE_CMD lsb_release)

	# Get distro codename
	if(NOT LSB_RELEASE_CMD)
		message(WARNING "DEB Generator: Can't find lsb_release in your path. Setting DISTRO_CODENAME to unknown.")
		set(DISTRO_CODENAME unknown)
	else()
		execute_process(COMMAND ${LSB_RELEASE_CMD} -cs
			OUTPUT_VARIABLE DISTRO_CODENAME
			OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif()

	# https://gitlab.kitware.com/cmake/cmake/issues/16517
	set(CPACK_COMPONENTS_ALL runtime)
	set(CPACK_DEB_COMPONENT_INSTALL "ON")

	# Set package name
	set(CPACK_DEBIAN_PACKAGE_NAME "signalwire-client-c2")

 	# Abuse Cmake component name
 	set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME ${CPACK_DEBIAN_PACKAGE_NAME})

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
	if(DPKG_CMD)
 		execute_process(COMMAND ${DPKG_CMD} --print-architecture OUTPUT_VARIABLE DEB_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
 		set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${DEB_ARCH})
 		set(CPACK_SYSTEM_NAME ${DEB_ARCH})
 		unset(DEB_ARCH)
 	endif()

	# Enforce Debian policy permission rules
	set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

	# Set a Debian compliant changelog footer
	if(DATE_CMD)
		execute_process(COMMAND ${DATE_CMD} -R OUTPUT_VARIABLE RFC2822_TIMESTAMP)
		# Two spaces between maintainer and timestamp is NOT a mistake
		set(CHANGELOG_FOOTER " -- ${CPACK_DEBIAN_PACKAGE_MAINTAINER}  ${RFC2822_TIMESTAMP}")
	endif()

	# Guess version release from environment variable
	# (usage of CPACK_DEBIAN_PACKAGE_RELEASE breaks ability to set CPACK_DEBIAN_PACKAGE_VERSION properly)
	if (NOT "$ENV{PACKAGE_RELEASE}" STREQUAL "")
		set(PACKAGE_RELEASE "$ENV{PACKAGE_RELEASE}")
	else()
		if(DATE_CMD)
			execute_process(COMMAND ${DATE_CMD} +%Y%m%d OUTPUT_VARIABLE DATE_YMD)
			set(PACKAGE_RELEASE ${DATE_YMD})
		else()
			set(PACKAGE_RELEASE "1")
		endif()
	endif()

	# Set package version
	set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION}-${PACKAGE_RELEASE}~${DISTRO_CODENAME})

	# Set debian file name format
	set(CPACK_DEBIAN_FILE_NAME "${PACKAGE_NAME}_${PROJECT_VERSION}-${PACKAGE_RELEASE}_${DISTRO_CODENAME}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")

	# Set a Debian compliant changelog header
	set(CHANGELOG_HEADER "signalwire-client-c2 (${CPACK_DEBIAN_PACKAGE_VERSION}) ${DISTRO_CODENAME}\; urgency=${CPACK_DEBIAN_PACKAGE_PRIORITY}")

	# Generate a Debian compliant changelog
	if(GIT_FOUND AND GZIP_CMD AND DATE_CMD)
		# Get git log info
		message("Detecting last git tag to generate a Debian complian changelog.")
		execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
			OUTPUT_VARIABLE LAST_TAG
			WORKING_DIRECTORY .
			OUTPUT_STRIP_TRAILING_WHITESPACE)

		# Commits count since last tag
		execute_process(COMMAND ${GIT_EXECUTABLE} rev-list ${LAST_TAG}.. --count
			OUTPUT_VARIABLE COMMITS_COUNT_SINCE_LAST_TAG
			WORKING_DIRECTORY .
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		message("Found last git tag: ${LAST_TAG}")

		# Check if we have commits since last tag
		if (COMMITS_COUNT_SINCE_LAST_TAG STREQUAL "0")
			# if not, find second tag so we could have a changelog
			message("No commits since git tag '${LAST_TAG}' to generate a changelog, looking for a previous tag")
			execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 ${LAST_TAG}^
				OUTPUT_VARIABLE LAST_TAG
				WORKING_DIRECTORY .
				OUTPUT_STRIP_TRAILING_WHITESPACE)
		endif()

		message("Generating a changelog using commits since git tag: ${LAST_TAG}")
		execute_process(COMMAND ${GIT_EXECUTABLE} log --no-merges --pretty=format:"%n  [%an]%n   * %s" ${LAST_TAG}..HEAD
			OUTPUT_VARIABLE CHANGELOG
			WORKING_DIRECTORY .
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		string(REPLACE "\"" "" CHANGELOG "${CHANGELOG}")

		# Create changelog
		file(WRITE changelog.Debian ${CHANGELOG_HEADER}\n${CHANGELOG}\n\n${CHANGELOG_FOOTER})
		execute_process(COMMAND ${GZIP_CMD} -f -9 -n changelog.Debian)

		# Install changelog
		install(FILES "${CMAKE_BINARY_DIR}/changelog.Debian.gz" COMPONENT "runtime" DESTINATION "share/doc/signalwire-client-c2")

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

add_library(signalwire_client2 SHARED ${SWCltSrc} ${SWCltInc})
set_target_properties(signalwire_client2 PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(signalwire_client2 PROPERTIES DEBUG_POSTFIX "")

# Define our exports symbol to key any definitions to toggle the visibility type
set_target_properties(signalwire_client2 PROPERTIES DEFINE_SYMBOL SWCLT_EXPORTS)

# Setup blade core definnitions
target_compile_definitions(signalwire_client2 PUBLIC
  -DSWCLT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  -DSWCLT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  -DSWCLT_VERSION_REVISION=${PROJECT_VERSION_PATCH}
)

if (KS_PLAT_WIN)
	target_compile_definitions(signalwire_client2 PUBLIC)
endif()

if (SWCLT_DEBUG_JSON)
	target_compile_definitions(signalwire_client2 PUBLIC
	  -DSWCLT_DEBUG_JSON=1
	)
endif()

# Copy our ca dir to the binary dir so we can run these things in the build env
if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/ca)
	file(COPY ${CMAKE_CURRENT_LIST_DIR}/ca DESTINATION ${CMAKE_BINARY_DIR})
endif()

# Setup ks core dependent linkages
if (NOT WIN32)
		target_link_libraries(signalwire_client2 PUBLIC ks2)
else()
		target_link_libraries(
			signalwire_client2 PUBLIC
			${LIBKS_LIBRARY_PATH}/ks2.lib
			OpenSSL::SSL
			OpenSSL::Crypto
		)
endif()

# Include key paths
target_include_directories(
  signalwire_client2 PUBLIC
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/libks2/libks>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
)

if (WIN32)
# Include key paths
		target_include_directories(
			signalwire_client2 PUBLIC
			$<BUILD_INTERFACE:;${LIBKS_INCLUDE_DIRS}/src/include>
			$<BUILD_INTERFACE:;${OPENSSL_INCLUDE_DIR}>
		)
endif()

target_include_directories(
	signalwire_client2 PRIVATE
)

# Include the bin dir for config discovery
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")

if (NOT KS_PLAT_WIN)
	# Set install targets
	install(TARGETS signalwire_client2 COMPONENT "runtime" EXPORT SignalWireClient2Config DESTINATION lib)
	install(DIRECTORY inc/signalwire-client-c COMPONENT "runtime" DESTINATION include/signalwire-client-c2 PATTERN internal EXCLUDE)

	# Set path for pkg-config based on ARCH and distro type
	if("${CMAKE_OS_NAME}" STREQUAL "Centos") # pkg-config --variable pc_path pkg-config
		if(${CPACK_SYSTEM_NAME} MATCHES "x86_64")
			install(FILES ${PROJECT_BINARY_DIR}/signalwire_client2.pc COMPONENT "runtime" DESTINATION lib64/pkgconfig)
		else()
			install(FILES ${PROJECT_BINARY_DIR}/signalwire_client2.pc COMPONENT "runtime" DESTINATION lib/pkgconfig)
		endif()
	else()
		install(FILES ${PROJECT_BINARY_DIR}/signalwire_client2.pc COMPONENT "runtime" DESTINATION lib/pkgconfig)
	endif()

	install(EXPORT SignalWireClient2Config COMPONENT "runtime" DESTINATION include/signalwire-client-c2/signalwire-client-c/cmake)
	install(FILES ${PROJECT_BINARY_DIR}/copyright COMPONENT "runtime" DESTINATION share/doc/signalwire-client-c2)

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
	get_property(PC_DEFINITIONS TARGET signalwire_client2 PROPERTY INTERFACE_COMPILE_DEFINITIONS)
	get_property(PC_DEFINITIONS TARGET ks2 PROPERTY INTERFACE_COMPILE_DEFINITIONS)
	set(PACKAGE_NAME signalwire_client2)
	set(PACKAGE_VERSION ${PROJECT_VERSION})
	configure_file("${CMAKE_CURRENT_LIST_DIR}/signalwire_client.pc.in" "${PROJECT_BINARY_DIR}/signalwire_client2.pc" @ONLY)
endif()

cotire(signalwire_client2)

enable_testing()
add_subdirectory(swclt_test)
