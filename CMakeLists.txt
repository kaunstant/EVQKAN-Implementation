cmake_minimum_required(VERSION 3.16)
project(QNN_App CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT MSVC)
    message(FATAL_ERROR "The bundled QCS 0.3.5 libraries are MSVC .lib files. Build with MSVC on Windows or provide a platform-native QCS package.")
endif()

set(QCS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/qcs" CACHE PATH "Root directory containing the bundled QCS builds")
set(QCS_DEBUG_ROOT "${QCS_ROOT}/debug")
set(QCS_RELEASE_ROOT "${QCS_ROOT}/release")

foreach(QCS_CONFIGURATION_ROOT IN ITEMS QCS_DEBUG_ROOT QCS_RELEASE_ROOT)
    if(NOT EXISTS "${${QCS_CONFIGURATION_ROOT}}/include/QCS")
        message(FATAL_ERROR "Missing QCS headers: ${${QCS_CONFIGURATION_ROOT}}/include/QCS")
    endif()
    if(NOT EXISTS "${${QCS_CONFIGURATION_ROOT}}/lib/quantum_engine.lib")
        message(FATAL_ERROR "Missing QCS library: ${${QCS_CONFIGURATION_ROOT}}/lib/quantum_engine.lib")
    endif()
endforeach()

add_library(QCS::QCS STATIC IMPORTED GLOBAL)
set_target_properties(QCS::QCS PROPERTIES
    IMPORTED_CONFIGURATIONS "DEBUG;RELEASE"
    IMPORTED_LOCATION_DEBUG "${QCS_DEBUG_ROOT}/lib/quantum_engine.lib"
    IMPORTED_LOCATION_RELEASE "${QCS_RELEASE_ROOT}/lib/quantum_engine.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    INTERFACE_INCLUDE_DIRECTORIES
        "$<$<CONFIG:Debug>:${QCS_DEBUG_ROOT}/include;${QCS_DEBUG_ROOT}/include/core;${QCS_DEBUG_ROOT}/include/registers;${QCS_DEBUG_ROOT}/include/algorithms;${QCS_DEBUG_ROOT}/include/qnn;${QCS_DEBUG_ROOT}/include/variational>;$<$<NOT:$<CONFIG:Debug>>:${QCS_RELEASE_ROOT}/include;${QCS_RELEASE_ROOT}/include/core;${QCS_RELEASE_ROOT}/include/registers;${QCS_RELEASE_ROOT}/include/algorithms;${QCS_RELEASE_ROOT}/include/qnn;${QCS_RELEASE_ROOT}/include/variational>"
)

# 1. Build the project-level KAN layer and executable.
add_library(KAN STATIC
    src/KAN/QuantumActivation.cpp
    src/KAN/QKAN.cpp
    src/KAN/EVQKAN.cpp
)

target_include_directories(KAN PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(QuantumSimulationStudio main.cpp)

# 2. Add all header search paths
target_include_directories(QuantumSimulationStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 3. Link the project to the configuration-specific bundled QCS build.
target_link_libraries(KAN PUBLIC QCS::QCS)
target_link_libraries(QuantumSimulationStudio PRIVATE KAN)