set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.8.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(Catch2)

add_executable(
    yk_rvariant_test

    # Independent Polyfill
    indirect_test.cpp

    # Independent language core / exposition-only utils
    core_test.cpp

    # Main library
    rvariant_test.cpp
    get_visit_test.cpp
    flexible_test.cpp
    recursive_wrapper_test.cpp
    truly_recursive_test.cpp
)

target_sources(yk_rvariant_test PRIVATE FILE_SET HEADERS FILES rvariant_test.hpp)

# Don't remove this; required for ABI compatibility
target_compile_features(Catch2 PUBLIC cxx_std_23)

target_compile_features(yk_rvariant_test PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(
        Catch2
            PUBLIC /fsanitize=address
            PRIVATE /MP /analyze-
    )
    target_link_options(
        Catch2
            PUBLIC /INCREMENTAL:NO # required for ASan
    )

    target_compile_definitions(
        yk_rvariant_test
            PUBLIC UNICODE _UNICODE
    )

    target_compile_options(
        yk_rvariant_test
            PUBLIC
                /MP /utf-8 /W3 /analyze /analyze:external-
    )
else()
    target_compile_options(
        yk_rvariant_test
            PUBLIC
                -Wall -Wextra -pedantic -fsanitize=undefined,address
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
