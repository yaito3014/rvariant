include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.8.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(Catch2)

add_executable(yk_rvariant_test test.cpp)

target_compile_features(yk_rvariant_test PRIVATE cxx_std_26)

target_link_libraries(
    yk_rvariant_test
    PRIVATE yk::rvariant Catch2::Catch2WithMain
)

add_test(NAME yk_rvariant_test COMMAND yk_rvariant_test)
