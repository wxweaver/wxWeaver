# wxWEAVER_DISABLE_MEDIACTRL
if(UNIX AND NOT wxWEAVER_DISABLE_MEDIACTRL)
    execute_process(
        COMMAND wx-config --libs all
        OUTPUT_VARIABLE _wxLibs
    )
    if(_wxLibs MATCHES "media")
        add_compile_definitions(USE_MEDIACTRL)
    else()
        set(wxWEAVER_DISABLE_MEDIACTRL ON)
    endif()
endif()

if(WIN32 AND NOT MSYS)
    list(APPEND wxLibsList gl core base net xml xrc html adv stc richtext propgrid ribbon aui)
    if(NOT wxWEAVER_DISABLE_MEDIACTRL)
        list(APPEND wxLibsList media)
    endif()
else()
    list(APPEND wxLibsList all)
endif()

function(copy_resources)
    file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/share")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/application/"
         DESTINATION "${CMAKE_BINARY_DIR}/share/wxweaver")
endfunction()

# https://cmake.org/pipermail/cmake/2008-January/019290.html
function(set_plugin_directory PATH)
    set(linkerOpt "-Wl,-rpath,$``ORIGIN/../${PATH}:$$``ORIGIN/../${PATH}")
    message(STATUS "rpath is ${linkerOpt}")
    set(CMAKE_EXE_LINKER_FLAGS
        ${CMAKE_EXE_LINKER_FLAGS} "${linkerOpt}")
endfunction()

find_package(wxWidgets 3.0.3 REQUIRED ${wxLibsList})
if(${wxWidgets_FOUND})
    include(${wxWidgets_USE_FILE})
    include(CMakeDependentOption)

    set(CMAKE_CXX_FLAGS_DEBUG "-DwxWEAVER_DEBUG ${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    if(MSVC)
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    endif()

    # The variable CMAKE_SYSTEM_PROCESSOR is incorrect on Visual studio, see
    # https://gitlab.kitware.com/cmake/cmake/issues/15170
    if(NOT wxWEAVER_SYSTEM_PROCESSOR)
        if(MSVC)
            set(wxWEAVER_SYSTEM_PROCESSOR "${MSVC_CXX_ARCHITECTURE_ID}")
        else()
            set(wxWEAVER_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    endif()

    # wxWEAVER_USE_LIBCPP: libc++ is enabled by default on macOS.
    cmake_dependent_option(wxWEAVER_USE_LIBCPP "Use libc++ with clang" "${APPLE}"
        "CMAKE_CXX_COMPILER_ID MATCHES Clang" OFF)
    if(wxWEAVER_USE_LIBCPP)
        add_compile_options(-stdlib=libc++)
        if(CMAKE_VERSION VERSION_LESS 3.13)
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
        else()
            add_link_options(-stdlib=libc++ -lc++abi)
        endif()
    endif()
endif()
