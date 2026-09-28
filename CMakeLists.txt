cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(SanDiX_TPC_GEANT4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Geant4 REQUIRED ui_all vis_all)
find_package(NEST REQUIRED)

include(${Geant4_USE_FILE})

file(GLOB SANDIX_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cc"
)

add_executable(sandix ${SANDIX_SOURCES})

target_include_directories(sandix PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    ${NEST_INCLUDE_DIRS}
)

target_link_libraries(sandix PRIVATE
    ${Geant4_LIBRARIES}
    NEST::NESTG4
)

file(GLOB SANDIX_MACROS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/macros/*.mac"
)

foreach(macro_file IN LISTS SANDIX_MACROS)
    get_filename_component(macro_name "${macro_file}" NAME)
    configure_file("${macro_file}" "${macro_name}" COPYONLY)
endforeach()

configure_file(
    "${PROJECT_SOURCE_DIR}/analysis/graphing.c"
    "${PROJECT_BINARY_DIR}/graphing.c"
    COPYONLY
)

add_custom_target(SanDiXSimulation DEPENDS sandix)
