cmake_minimum_required(VERSION 3.23)

project(rvariant LANGUAGES CXX)

add_library(rvariant INTERFACE)

target_compile_features(rvariant INTERFACE cxx_std_23)

file(GLOB YK_RVARIANT_HEADERS include/**/*.hpp)
target_sources(
    rvariant
    INTERFACE FILE_SET HEADERS BASE_DIRS include FILES ${YK_RVARIANT_HEADERS}
)

add_library(yk::rvariant ALIAS rvariant)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()
endif()
