# LIBKS_FOUND
# LIBKS_LIBRARIES
# LIBKS_LIBRARY_DIRS
# LIBKS_LDFLAGS
# LIBKS_INCLUDE_DIRS
# LIBKS_CFLAGS
# LIBKS_CMAKE_DIR

if (NOT TARGET ks)
	if (NOT WIN32)
		include(FindPkgConfig)

		# Use the pkg-config system to locate our libks configs
		pkg_check_modules(LIBKS libks2 REQUIRED)

		# From here we can bootstrap into cmake stuff
		set(LIBKS_CMAKE_DIR ${LIBKS_INCLUDE_DIRS}/libks/cmake)
	else()
		if ("${LIBKS_INCLUDE_DIRS}" STREQUAL "")
			message(FATAL_ERROR "Can't find Libks. Try setting LIBKS_INCLUDE_DIRS.")
		endif()

		# From here we can bootstrap into cmake stuff
		set(LIBKS_CMAKE_DIR ${LIBKS_INCLUDE_DIRS}/cmake)
	endif()

	# Load ks utils for our build macros
	include(${LIBKS_CMAKE_DIR}/ksutil.cmake)

	# Now load the package with a hint on where to look
	set(LibKS2_DIR ${LIBKS_CMAKE_DIR})

	if (NOT WIN32)
		find_package(LibKS2 REQUIRED VERSION)
	else()
		if(LIBKS_INCLUDE_DIRS AND EXISTS "${LIBKS_INCLUDE_DIRS}/CMakeLists.txt")
			file(STRINGS "${LIBKS_INCLUDE_DIRS}/CMakeLists.txt" libks_version_str
			REGEX "^[\t ]*project[\t (]*LibKS2[\t ]+VERSION[\t ]+([0-9a-fA-F]\.[0-9a-fA-F]).*")

			string(COMPARE EQUAL "${libks_version_str}" "" _is_empty)
			if(_is_empty)
				message(
					FATAL_ERROR
					"Incorrect LibKS VERSION in project command of LibKS's CMakeLists.txt"
					": ${LIBKS_INCLUDE_DIRS}/CMakeLists.txt")
			endif()
			string(REGEX REPLACE "^[\t ]*project[\t (]*LibKS2[\t ]+VERSION[\t ]+([0-9a-fA-F]\.[0-9a-fA-F]).*$"
				"\\1;" LIBKS_VERSION_LIST "${libks_version_str}")
			list(GET LIBKS_VERSION_LIST 0 LIBKS_VERSION)
		endif()
	endif()

	message("Found LibKS2 ${LIBKS_VERSION} package at path ${LIBKS_INCLUDE_DIRS}")
	if (${LIBKS_VERSION} VERSION_LESS 2.0)
		message(FATAL "Libks version 2.0 or greater is required")
	endif()
else()
	get_target_property(KS_SOURCE_DIR ks2 SOURCE_DIR)
	message("Switchblade ${KS_SOURCE_DIR}")
	set(LIBKS_CMAKE_DIR ${KS_SOURCE_DIR}/cmake)

	# Load ks utils for our build macros
	include(${LIBKS_CMAKE_DIR}/ksutil.cmake)
endif()

# Let ks define some fundamentals across all cmake setups
ksutil_setup_platform()

# We'll need to load openssl as well
if (KS_PLAT_MAC)
	set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
endif()

if (WIN32)
	include(FindOpenSSL)
endif()

find_package(OpenSSL REQUIRED Crypto SSL)
