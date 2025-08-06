# Copyright 2025 Yaito Kakeyama
# Copyright 2025 Nana Sakisaka
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.23)

project(rvariant LANGUAGES CXX)
set(YK_RVARIANT_CXX_FEATURE cxx_std_23)

add_library(yk_rvariant_cxx_common INTERFACE)
target_compile_features(yk_rvariant_cxx_common INTERFACE ${YK_RVARIANT_CXX_FEATURE})
set_target_properties(yk_rvariant_cxx_common PROPERTIES CXX_EXTENSIONS OFF)

if(MSVC)
    list(REMOVE_ITEM CMAKE_CONFIGURATION_TYPES MinSizeRel)

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

add_library(rvariant INTERFACE)
add_library(yk::rvariant ALIAS rvariant)
set_target_properties(rvariant PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(rvariant INTERFACE yk_rvariant_cxx_common)

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
    PRIVATE ${YK_RVARIANT_HEADERS}
    INTERFACE FILE_SET HEADERS FILES ${YK_RVARIANT_HEADERS}
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
