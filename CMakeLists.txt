# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# use new policy
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0003 NEW)
if(POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW)
endif()

# enable testing
enable_testing()

# project name, version, and languages
project(RisohEditor VERSION 6.1.5 LANGUAGES CXX RC)

# PROJECT_VERSION macro
add_definitions(-DPROJECT_VERSION=\"${PROJECT_VERSION}\")

# set output directory (build/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# define RISOHEDITOR
add_definitions(-DRISOHEDITOR)

# statically link
if (WIN32)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# using Clang
		set(CMAKE_C_FLAGS "-static -W -Wno-missing-field-initializers")
		set(CMAKE_CXX_FLAGS "-static -W -Wno-missing-field-initializers")
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		# using GCC
		set(CMAKE_C_FLAGS "-static -W -Wno-missing-field-initializers")
		set(CMAKE_CXX_FLAGS "-static -W -Wno-missing-field-initializers")
	elseif (MSVC)
		# use explicit runtime library settings (/MTd in Debug, /MT in others)
		if(POLICY CMP0091)
			set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
		else()
			# fallback for older CMake versions
			set(CompilerFlags
				CMAKE_C_FLAGS
				CMAKE_C_FLAGS_DEBUG
				CMAKE_C_FLAGS_RELEASE
				CMAKE_C_FLAGS_MINSIZEREL
				CMAKE_C_FLAGS_RELWITHDEBINFO
				CMAKE_CXX_FLAGS
				CMAKE_CXX_FLAGS_DEBUG
				CMAKE_CXX_FLAGS_RELEASE
				CMAKE_CXX_FLAGS_MINSIZEREL
				CMAKE_CXX_FLAGS_RELWITHDEBINFO)
			foreach(CompilerFlags ${CompilerFlags})
				string(REPLACE "/MD" "/MT" ${CompilerFlags} "${${CompilerFlags}}")
			endforeach()
		endif()
	endif()
endif()

# ENABLE_CRYPTO option
option(ENABLE_CRYPTO "Support crypto" ON)
set(WONRES_ENABLE_CRYPTO ${ENABLE_CRYPTO} CACHE BOOL "Enable WonRes encryption" FORCE)
message(STATUS "ENABLE_CRYPTO: ${ENABLE_CRYPTO}")
if(ENABLE_CRYPTO)
    add_definitions(-DENABLE_CRYPTO)
endif()

# PROTECTION option
option(PROTECTION "Software Protection on Release" OFF)
message(STATUS "PROTECTION: ${PROTECTION}")
if(PROTECTION)
    add_definitions(-DPROTECTION)
endif()

##############################################################################

include_directories(WonSetThreadUILanguage)

add_subdirectory(WonRes)
if(MSVC AND TARGET WonRes AND DEFINED CMAKE_MSVC_RUNTIME_LIBRARY AND NOT CMAKE_MSVC_RUNTIME_LIBRARY STREQUAL "")
	set_property(TARGET WonRes PROPERTY MSVC_RUNTIME_LIBRARY "${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()

add_definitions(-DMZC4_HANDLE_MAP=1)
add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NON_CONFORMING_WCSTOK -D_SCL_SECURE_NO_WARNINGS)

include_directories(EGA)
add_subdirectory(EGA)

include_directories(LineNumEdit)
add_subdirectory(LineNumEdit)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(mcdx)
add_subdirectory(MyWndCtrl)

##############################################################################
