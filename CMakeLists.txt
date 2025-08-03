cmake_minimum_required(VERSION 3.23)

project(rvariant LANGUAGES CXX)

add_library(rvariant INTERFACE)

target_compile_features(rvariant INTERFACE cxx_std_23)
set_target_properties(rvariant PROPERTIES CXX_EXTENSIONS OFF)

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

add_library(yk::rvariant ALIAS rvariant)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)

    if(BUILD_TESTING)
        add_subdirectory(test)

        if(MSVC)
            set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT yk_rvariant_test)
        endif()
    endif()
endif()
