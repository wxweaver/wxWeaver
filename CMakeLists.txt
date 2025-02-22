cmake_minimum_required(VERSION 3.15)
project(wxWeaver
    VERSION 0.1.0
    LANGUAGES CXX C
    DESCRIPTION "GUI Designer for wxWidgets"
    HOMEPAGE_URL "http://wxweaver.github.io"
)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/scripts/cmake)

set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard to be used")
set(CMAKE_C_STANDARD 99 CACHE STRING "C standard to be used")

option(wxWEAVER_DISABLE_MEDIACTRL "Disable wxMediaCtrl / wxMedia library. [Default: OFF]" OFF)

# TODO: Custom wxWidgets build
#option(wxWEAVER_DISABLE_SHARED    "Use static wxWidgets build instead of shared libraries. [Default: OFF]" OFF)
#set(wxWEAVER_WX_ROOT "" CACHE STRING "Install destination for VST bundle [Default: Empty]")

include(wxWeaverConfig)

if(${wxWidgets_FOUND})
    add_subdirectory(sdk)
    include(wxWeaverLibs)
    include(wxWeaver)

    if(UNIX)
        if(NOT APPLE)
            # Local resources setup
            set_plugin_directory("lib/wxweaver")
        endif()
        copy_resources()
    endif()

    message(STATUS "
CMake Generator:             ${CMAKE_GENERATOR}

Project name:                ${PROJECT_NAME}
Project version:             ${PROJECT_VERSION}
Build type:                  ${CMAKE_BUILD_TYPE}
Build processor:             ${wxWEAVER_SYSTEM_PROCESSOR}
Use clang libc++:            ${wxWEAVER_USE_LIBCPP}
Install prefix:              ${CMAKE_INSTALL_PREFIX}
Output directory:            ${CMAKE_BINARY_DIR}
Disable wxMediaCtrl:         ${wxWEAVER_DISABLE_MEDIACTRL}

wxWidgets version:           ${wxWidgets_VERSION_STRING}
wxWidgets static:            ${wxWidgets_DEFAULT_STATIC}
wxWidgets debug:             ${wxWidgets_DEFAULT_DEBUG}
wxWidgets unicode:           ${wxWidgets_DEFAULT_UNICODE}
wxWidgets CXX flags:         ${wxWidgets_CXX_FLAGS_LIST}

Compiler CXX debug flags:    ${CMAKE_CXX_FLAGS_DEBUG}
Compiler CXX release flags:  ${CMAKE_CXX_FLAGS_RELEASE}
Compiler CXX min size flags: ${CMAKE_CXX_FLAGS_MINSIZEREL}
")
endif()

#include(Utils)
#print_all_variables()
