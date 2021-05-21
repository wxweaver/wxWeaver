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
            sdk::ticpp
            sdk::plugin_interface
            ${wxWidgets_LIBRARIES}
        )
    endforeach()
endfunction()
_add_plugins()
