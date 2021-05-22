# Plugins
set(wxWeaverPlugins additional common containers forms layout)
set(additional_files
    src/plugins/additional/additional.cpp
)
set(common_files
    src/plugins/common/common.cpp
    src/plugins/common/imagelist.h
)
set(containers_files
    src/plugins/containers/bookutils.h
    src/plugins/containers/bookctrls.h
    src/plugins/containers/containers.cpp
)
set(forms_files
    src/plugins/forms/forms.cpp
)
set(layout_files
    src/plugins/layout/layout.cpp
)

function(_add_plugins)
    foreach(_plugin IN LISTS wxWeaverPlugins)
        add_library(${_plugin} MODULE ${${_plugin}_files})
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
