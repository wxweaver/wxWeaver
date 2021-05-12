# TICPP
add_subdirectory(external/ticpp)
add_library(wxweaver::ticpp ALIAS ticpp)

# Plugin Interface
set(wxWEAVER_PLUGIN_INTERFACE_SOURCE_FILES
    sdk/plugin_interface/component.h
    sdk/plugin_interface/fontcontainer.h
    sdk/plugin_interface/plugin.h
    sdk/plugin_interface/xrcconv.h
    sdk/plugin_interface/xrcconv.cpp
    sdk/plugin_interface/forms/wizard.h
    sdk/plugin_interface/forms/wizard.cpp
    sdk/plugin_interface/forms/wizard.fbp
)
add_library(plugin_interface STATIC ${wxWEAVER_PLUGIN_INTERFACE_SOURCE_FILES})
add_library(wxweaver::plugin_interface ALIAS plugin_interface)
if(MSVC)
    # Workaround to unwanted build-type directory added by MSVC
    set_target_properties(plugin_interface PROPERTIES
        SUFFIX ".lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<0:>"
    )
endif()
target_include_directories(plugin_interface PUBLIC "sdk/plugin_interface")
target_link_libraries(plugin_interface wxweaver::ticpp ${wxWidgets_LIBRARIES})

# Plugins
set(wxWeaverPlugins additional common containers forms layout)
function(_add_plugins)
    foreach(_plugin IN LISTS wxWeaverPlugins)
        add_library(${_plugin} MODULE "src/plugins/${_plugin}/${_plugin}.cpp")
        add_library(wxweaver::${_plugin} ALIAS ${_plugin})
        if(WIN32)
            set_target_properties(${_plugin} PROPERTIES
                PREFIX "lib"
                SUFFIX ".dll"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/${_plugin}/$<0:>"
            )
        else()
            set_target_properties(${_plugin} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/wxweaver"
            )
        endif()
        target_compile_definitions(${_plugin} PRIVATE BUILD_DLL)
        target_include_directories(${_plugin} PRIVATE "sdk/plugin_interface")
        target_link_libraries(${_plugin}
            wxweaver::ticpp
            wxweaver::plugin_interface
            ${wxWidgets_LIBRARIES}
        )
    endforeach()
endfunction()
_add_plugins()
