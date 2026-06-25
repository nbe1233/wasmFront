cmake_minimum_required(VERSION 3.10)
project(wasmFront C)
option(CMAKE_BUILD_TYPE "Build Type" Debug)

include(FetchContent)



FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "5.5"
    GIT_PROGRESS TRUE
)

FetchContent_Declare(
    wasmi
    GIT_REPOSITORY "https://github.com/wasmi-labs/wasmi.git"
    GIT_TAG "main"
    GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(raylib)
add_subdirectory(wasmi/crates/c_api )
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src/")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib wasmi)
