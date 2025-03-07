# wxWEAVER_DISABLE_MEDIACTRL
# TODO: Check macOS brew if has wxMediaCtrl.
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

if(UNIX)
    # Copy the resources in the shared application folder.
    # On Linux set rpath linker path. TODO: use dlopen() instead.
    if(APPLE)
        function(copy_resources)
            file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/wxWeaver.app/Contents/SharedSupport")
            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/application/"
                 DESTINATION "${CMAKE_BINARY_DIR}/wxWeaver.app/Contents/SharedSupport")
        endfunction()
    else()
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
    endif()
endif()

# Compile translations and setup the install destination.
# The resulting installed files will have .mo extension.
include(FindGettext)
if(${GETTEXT_FOUND})
    set(languages it)
    set(translations wxweaver libadditional libcommon libcontainers libforms liblayout)
    set(languagesDir "${CMAKE_CURRENT_SOURCE_DIR}/resources/i18n/locale")
    foreach(language ${languages})
        foreach(translation ${translations})
            gettext_process_po_files(${language} ALL
                INSTALL_DESTINATION "share/wxweaver/locale"
                PO_FILES "${languagesDir}/${language}/${translation}.po"
            )
        endforeach()
    endforeach()
endif()

# Copy translation files to the build directory when the application is not installed.
# wxWidgets currently (v3.1.6) supports only .mo extension, not .gmo,
# which is the one used by CMake.
function(target_copy_translation target language)
    if(NOT ${GETTEXT_FOUND})
        message(STATUS "gettext not found, not copying files for target ${target}")
        return()
    endif()
    set(translation ${target})
    if(${target} STREQUAL "wxWeaver")
        set(translation "wxweaver")
    else()
        set(translation "lib${target}")
    endif()
    set(sourceFileName "${CMAKE_BINARY_DIR}/${translation}.gmo")
    set(destinationDir "${CMAKE_BINARY_DIR}/share/wxweaver/locale/${language}/LC_MESSAGES")
    if(NOT EXISTS ${destinationDir})
        add_custom_command(TARGET ${target} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "${destinationDir}"
        )
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${sourceFileName}"
        "${destinationDir}/${translation}.mo"
        COMMENT "Copying ${translation}.mo to ${destinationDir}"
    )
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
