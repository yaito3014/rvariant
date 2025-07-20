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
    indirect_test.cpp
    rvariant_test.cpp
    flexible_test.cpp
    recursive_test.cpp
)

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
                /MP /utf-8 /W3 /analyze /analyze:external- /external:anglebrackets
    )
endif()

target_link_libraries(
    yk_rvariant_test
    PRIVATE yk::rvariant Catch2::Catch2WithMain
)

add_test(NAME yk_rvariant_test COMMAND yk_rvariant_test)
