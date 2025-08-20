# Copyright 2025 Yaito Kakeyama
# Copyright 2025 Nana Sakisaka
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# https://github.com/catchorg/Catch2/blob/devel/docs/configuration.md
set(CATCH_CONFIG_FAST_COMPILE ON)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.8.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(Catch2)

set_target_properties(Catch2 PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(Catch2 PRIVATE yk_rvariant_cxx_common)
target_compile_definitions(Catch2 PUBLIC DO_NOT_USE_WMAIN)

set_target_properties(Catch2WithMain PROPERTIES CXX_EXTENSIONS OFF)
target_link_libraries(Catch2WithMain PRIVATE yk_rvariant_cxx_common)

# ---------------------------------------

add_executable(
    yk_rvariant_test

    # Independent Polyfill
    indirect_test.cpp

    # Independent language core / exposition-only utils
    core_test.cpp

    # Main library
    rvariant_test.cpp
    many_alternatives_32_test.cpp
    get_visit_test.cpp
    flexible_test.cpp
    recursive_wrapper_test.cpp
    truly_recursive_test.cpp
    io_test.cpp
)

if(MSVC)
    target_sources(yk_rvariant_test PRIVATE cpp.hint)
endif()

target_sources(
    yk_rvariant_test
    PRIVATE FILE_SET HEADERS FILES
        rvariant_test.hpp
)

set_target_properties(
    yk_rvariant_test
    PROPERTIES CXX_EXTENSIONS OFF
)

if(MSVC)
    target_compile_options(
        Catch2
        PUBLIC
            /wd5072 # ASan intentionally enabled on Release build
            /fsanitize=address
        PRIVATE
            /analyze-
    )
    target_link_options(
        Catch2
        PUBLIC
            /ignore:4302 # ASan intentionally enabled on Release build
            /INCREMENTAL:NO # required for ASan
    )

    target_compile_options(
        yk_rvariant_test
        PUBLIC
            /W4 /analyze /analyze:external-
            /fsanitize=address
    )
else()
    target_compile_options(
        yk_rvariant_test
        PUBLIC
            -fsanitize=undefined,address
    )
    target_link_options(
        yk_rvariant_test
        PUBLIC
            -fsanitize=undefined,address
    )
endif()

target_link_libraries(
    yk_rvariant_test
    PRIVATE yk::rvariant Catch2::Catch2WithMain
)

add_test(NAME yk_rvariant_test COMMAND yk_rvariant_test)

# ------------------------------------------------
# benchmark

add_library(
    yk_rvariant_benchmark_support
    SHARED benchmark_support.cpp
)
target_sources(
    yk_rvariant_benchmark_support
    PRIVATE FILE_SET HEADERS FILES
        benchmark_support.hpp
)

if(MSVC)
    target_compile_options(
        yk_rvariant_benchmark_support
        PUBLIC
            /W4 /analyze /analyze:external-
            $<$<CONFIG:Release,MinSizeRel>:/Oi> # intrinsic
    )
else() # non-MSVC
    target_compile_options(
        yk_rvariant_benchmark_support
        PUBLIC
            $<$<CONFIG:Release>:-march=native> # intrinsic
    )
endif()

set_target_properties(
    yk_rvariant_benchmark_support
    PROPERTIES CXX_EXTENSIONS OFF
)
target_link_libraries(
    yk_rvariant_benchmark_support
    PRIVATE yk_rvariant_cxx_common
)

add_executable(
    yk_rvariant_benchmark
    benchmark.cpp
)
set_target_properties(
    yk_rvariant_benchmark
    PROPERTIES CXX_EXTENSIONS OFF
)
target_link_libraries(
    yk_rvariant_benchmark
    PRIVATE yk_rvariant_benchmark_support
    PRIVATE yk::rvariant
)
