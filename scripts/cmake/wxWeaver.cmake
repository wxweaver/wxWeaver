set(wxWEAVER_INCLUDE_FILES
    external/stacktrace/stack.hpp
    external/md5/md5.hh
    src/codegen/codegen.h
    src/codegen/codeparser.h
    src/codegen/codewriter.h
    src/codegen/cppcg.h
    src/codegen/luacg.h
    src/codegen/phpcg.h
    src/codegen/pythoncg.h
    src/codegen/xrccg.h
    src/gui/aui/barart.h
    src/gui/aui/dockart.h
    src/gui/aui/tabart.h
    src/gui/dialogs/about.h
    src/gui/dialogs/annoying.h
    src/gui/dialogs/geninheritclass/geninhertclass.h
    src/gui/dialogs/geninheritclass/geninhertclass_gui.h
    src/gui/dialogs/geninheritclass/GenInheritedDlg.fbp
    src/gui/dialogs/menueditor.h
    src/gui/dialogs/preferences.h
    src/gui/dialogs/xrcpreview.h
    src/gui/panels/codeeditor/codeeditor.h
    src/gui/panels/codeeditor/plugins/cpp.h
    src/gui/panels/codeeditor/plugins/lua.h
    src/gui/panels/codeeditor/plugins/php.h
    src/gui/panels/codeeditor/plugins/python.h
    src/gui/panels/codeeditor/plugins/xrc.h
    src/gui/panels/debugwindow.h
    src/gui/panels/designer/innerframe.h
    src/gui/panels/designer/menubar.h
    src/gui/panels/designer/visualeditor.h
    src/gui/panels/designer/visualobj.h
    src/gui/panels/designer/window_buttons.h
    src/gui/panels/inspector/advprops.h
    src/gui/panels/inspector/inspector.h
    src/gui/panels/palette.h
    src/gui/panels/title.h
    src/gui/panels/treeview.h
#   src/gui/unused/customkeys.h
#   src/gui/unused/resizablepanel.h
    src/gui/bitmaps.h
    src/gui/mainframe.h
    src/model/database.h
    src/model/objectbase.h
    src/model/types.h
    src/model/xrcfilter.h
    src/utils/debug.h
    src/utils/defs.h
    src/utils/exception.h
    src/utils/filetocarray.h
    src/utils/ipc.h
    src/utils/stringutils.h
    src/utils/typeconv.h
    src/appdata.h
    src/cmdproc.h
    src/dataobject.h
    src/event.h
    src/manager.h
    src/settings.h
)
set(wxWEAVER_SOURCE_FILES
    external/stacktrace/stack.cpp
    external/md5/md5.cc
    src/codegen/codegen.cpp
    src/codegen/codeparser.cpp
    src/codegen/codewriter.cpp
    src/codegen/cppcg.cpp
    src/codegen/luacg.cpp
    src/codegen/phpcg.cpp
    src/codegen/pythoncg.cpp
    src/codegen/xrccg.cpp
    src/gui/aui/barart.cpp
    src/gui/aui/dockart.cpp
    src/gui/aui/tabart.cpp
    src/gui/dialogs/about.cpp
    src/gui/dialogs/annoying.cpp
    src/gui/dialogs/geninheritclass/geninhertclass.cpp
    src/gui/dialogs/geninheritclass/geninhertclass_gui.cpp
    src/gui/dialogs/menueditor.cpp
    src/gui/dialogs/preferences.cpp
    src/gui/dialogs/xrcpreview.cpp
    src/gui/panels/codeeditor/codeeditor.cpp
    src/gui/panels/codeeditor/plugins/cpp.cpp
    src/gui/panels/codeeditor/plugins/lua.cpp
    src/gui/panels/codeeditor/plugins/php.cpp
    src/gui/panels/codeeditor/plugins/python.cpp
    src/gui/panels/codeeditor/plugins/xrc.cpp
    src/gui/panels/debugwindow.cpp
    src/gui/panels/designer/innerframe.cpp
    src/gui/panels/designer/menubar.cpp
    src/gui/panels/designer/visualeditor.cpp
    src/gui/panels/designer/visualobj.cpp
    src/gui/panels/inspector/advprops.cpp
    src/gui/panels/inspector/inspector.cpp
    src/gui/panels/palette.cpp
    src/gui/panels/title.cpp
    src/gui/panels/treeview.cpp
#   src/gui/unused/customkeys.cpp
#   src/gui/unused/resizablepanel.cpp TODO: ???
    src/gui/bitmaps.cpp
    src/gui/mainframe.cpp
    src/model/database.cpp
    src/model/objectbase.cpp
    src/model/types.cpp
    src/model/xrcfilter.cpp
    src/utils/filetocarray.cpp
    src/utils/ipc.cpp
    src/utils/m_wxweaver.cpp
    src/utils/stringutils.cpp
    src/utils/typeconv.cpp
    src/appdata.cpp
    src/cmdproc.cpp
    src/dataobject.cpp
    src/event.cpp
    src/manager.cpp
    src/settings.cpp
    src/wxweaver.cpp
)
if(APPLE)
   set(MACOSX_BUNDLE_ICON_FILE icon.icns)
   set(wxWEAVER_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/icon.icns")
   set_source_files_properties(${wxWEAVER_APP_ICON} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources"
   )
    add_executable(${CMAKE_PROJECT_NAME}
        MACOSX_BUNDLE
        ${wxWEAVER_INCLUDE_FILES}
        ${wxWEAVER_SOURCE_FILES}
        ${wxWEAVER_RESOURCE_FILES}
        ${wxWEAVER_APP_ICON}
    )
elseif(WIN32)
    list(APPEND wxWEAVER_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/windows/wxWeaver.rc")

    add_executable(${CMAKE_PROJECT_NAME}
        WIN32
        ${wxWEAVER_INCLUDE_FILES}
        ${wxWEAVER_SOURCE_FILES}
        ${wxWEAVER_RESOURCE_FILES}
    )
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        SUFFIX ".exe"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<0:>"
    )
    if(MSVC)
        # TODO: manager.cpp CHECK_{VISUAL_EDITOR|WX_OBJECT|OBJECT_BASE} macros warnings
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4003")
        target_link_libraries(${CMAKE_PROJECT_NAME} Dbghelp)
    else()
        find_library(BFD_LIB NAMES binutils/libbfd.a libbfd.a)
        find_library(IBERTY_LIB NAMES binutils/libiberty.a libiberty.a)
        find_package(ZLIB REQUIRED)
        find_package(Intl REQUIRED)

        target_link_libraries(${CMAKE_PROJECT_NAME}
            ${BFD_LIB}
            ${IBERTY_LIB}
            ${Intl_LIBRARIES}
            ${ZLIB_LIBRARIES}
            psapi
            imagehlp)
    endif()
else()
    add_executable(${CMAKE_PROJECT_NAME}
        ${wxWEAVER_INCLUDE_FILES}
        ${wxWEAVER_SOURCE_FILES}
        ${wxWEAVER_RESOURCE_FILES}
    )
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "wxweaver"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

if(MINGW)
    find_path(BINUTILS_BASE_INCLUDE_DIR binutils/bfd.h)
    target_include_directories(${CMAKE_PROJECT_NAME}
        PRIVATE ${BINUTILS_BASE_INCLUDE_DIR}/binutils
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "external"
    "sdk/plugin_interface"
    "src"
)
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${wxWidgets_LIBRARIES}
    sdk::ticpp
    sdk::plugin_interface
)
if(UNIX AND NOT APPLE)
    target_link_libraries(${CMAKE_PROJECT_NAME} dl)
endif()

target_copy_translation(${CMAKE_PROJECT_NAME} "it")

# Installation
if (UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    install(TARGETS ${wxWeaverPlugins}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/wxweaver"
    )
    install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/icons/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor"
    )
    install (FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/wxweaver.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
    )
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/wxweaver.sharedmimeinfo"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages"
        RENAME "wxweaver.xml"
    )
    install (FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/wxweaver.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )
    install(DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/application/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/wxweaver"
    )
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/wxweaver"
    )
endif()
