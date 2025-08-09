# Copyright 2025 Yaito Kakeyama
# Copyright 2025 Nana Sakisaka
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.23)

project(rvariant LANGUAGES CXX)

add_library(yk_rvariant_cxx_common INTERFACE)
set_target_properties(yk_rvariant_cxx_common PROPERTIES CXX_EXTENSIONS OFF)

# ----------------------------------------------

# This MUST be `OBJECT` target.
# CMake fails to determine the `/std:` flag and rollbacks to its
# default value regardless of how we set the value on `INTERFACE`
# target. Believe me, I tried setting every combination of target
# properties but none of them worked.
if(MSVC)
    add_library(rvariant OBJECT EXCLUDE_FROM_ALL)
    target_sources(
        rvariant
        PRIVATE
            # required for determining correct `/std:` flag
            "${PROJECT_SOURCE_DIR}/test/dummy.cpp"
    )
else()
    add_library(rvariant INTERFACE)
endif()

add_library(yk::rvariant ALIAS rvariant)

set_target_properties(rvariant PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(rvariant INTERFACE yk_rvariant_cxx_common)

# ----------------------------------------------

# Set minimal C++ version explicitly; avoid "/std:c++latest"
set(YK_RVARIANT_CXX_FEATURE cxx_std_23)
if(MSVC)
    if(CMAKE_CXX_STANDARD EQUAL 23)
        set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "/std:c++23preview")
        set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION "/std:c++23preview")
        target_compile_options(yk_rvariant_cxx_common INTERFACE /std:c++23preview)
        target_compile_options(rvariant PRIVATE /std:c++23preview)
    else()
        # may become /std:c++latest only on this case
        target_compile_features(yk_rvariant_cxx_common INTERFACE ${YK_RVARIANT_CXX_FEATURE})
        target_link_libraries(rvariant PRIVATE yk_rvariant_cxx_common)
    endif()
else()
    target_compile_features(yk_rvariant_cxx_common INTERFACE ${YK_RVARIANT_CXX_FEATURE})
endif()
unset(YK_RVARIANT_CXX_FEATURE)

if(MSVC)
    if(PROJECT_IS_TOP_LEVEL)
        list(REMOVE_ITEM CMAKE_CONFIGURATION_TYPES MinSizeRel)
    endif()

    target_compile_options(
        yk_rvariant_cxx_common
        INTERFACE /MP /utf-8
    )
    target_compile_definitions(
        yk_rvariant_cxx_common
        INTERFACE UNICODE _UNICODE
    )
else()
    target_compile_options(
        yk_rvariant_cxx_common
        INTERFACE -Wall -Wextra -pedantic
    )
endif()

# --------------------------------------------------

option(YK_CI "Enables intensive testing for CI" OFF)
if(YK_CI)
    target_compile_definitions(rvariant INTERFACE YK_CI=1)
endif()

if(MSVC)
    target_compile_options(rvariant INTERFACE /Zc:preprocessor /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES Clang)
    target_compile_options(rvariant INTERFACE -fno-builtin-std-forward_like)
endif()

file(GLOB_RECURSE YK_RVARIANT_HEADERS ${PROJECT_SOURCE_DIR}/include/*.hpp)

target_include_directories(
    rvariant
    INTERFACE ${PROJECT_SOURCE_DIR}/include
)

target_sources(
    rvariant
    PRIVATE FILE_SET HEADERS TYPE HEADERS FILES ${YK_RVARIANT_HEADERS}
)
source_group(
    TREE ${PROJECT_SOURCE_DIR}/include FILES ${YK_RVARIANT_HEADERS}
)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)

    if(BUILD_TESTING)
        add_subdirectory(test)

        if(MSVC)
            set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT yk_rvariant_test)
        endif()
    endif()
endif()
