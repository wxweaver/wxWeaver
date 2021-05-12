set(wxWEAVER_INCLUDE_FILES
    src/codegen/codegen.h
    src/codegen/codeparser.h
    src/codegen/codewriter.h
    src/codegen/cppcg.h
    src/codegen/luacg.h
    src/codegen/phpcg.h
    src/codegen/pythoncg.h
    src/codegen/xrccg.h
    src/dbg_stack_trace/stack.hpp
    src/md5/md5.hh
    src/model/database.h
    src/model/objectbase.h
    src/model/types.h
    src/model/xrcfilter.h
    src/rad/codeeditor/codeeditor.h
    src/rad/cpppanel/cpppanel.h
    src/rad/dataobject/dataobject.h
    src/rad/designer/innerframe.h
    src/rad/designer/menubar.h
#   src/rad/designer/resizablepanel.h
    src/rad/designer/visualeditor.h
    src/rad/designer/visualobj.h
    src/rad/designer/window_buttons.h
    src/rad/geninheritclass/geninhertclass.h
    src/rad/geninheritclass/geninhertclass_gui.h
    src/rad/inspector/advprops.h
    src/rad/inspector/objinspect.h
    src/rad/luapanel/luapanel.h
    src/rad/objecttree/objecttree.h
    src/rad/phppanel/phppanel.h
    src/rad/pythonpanel/pythonpanel.h
    src/rad/xrcpanel/xrcpanel.h
    src/rad/xrcpreview/xrcpreview.h
    src/rad/about.h
    src/rad/appdata.h
    src/rad/auitabart.h
    src/rad/bitmaps.h
    src/rad/cmdproc.h
    src/rad/customkeys.h
    src/rad/event.h
    src/rad/genericpanel.h
    src/rad/mainframe.h
    src/rad/manager.h
    src/rad/menueditor.h
    src/rad/palette.h
    src/rad/title.h
    src/utils/annoyingdialog.h
    src/utils/debug.h
    src/utils/defs.h
    src/utils/exception.h
    src/utils/filetocarray.h
    src/utils/ipc.h
    src/utils/wxlogstring.h
    src/utils/stringutils.h
    src/utils/typeconv.h
    src/maingui.h
    src/pch.h
    src/rad/geninheritclass/GenInheritedDlg.fbp
)
set(wxWEAVER_SOURCE_FILES
    src/codegen/codegen.cpp
    src/codegen/codeparser.cpp
    src/codegen/codewriter.cpp
    src/codegen/cppcg.cpp
    src/codegen/luacg.cpp
    src/codegen/phpcg.cpp
    src/codegen/pythoncg.cpp
    src/codegen/xrccg.cpp
    src/dbg_stack_trace/stack.cpp
    src/md5/md5.cc
    src/model/database.cpp
    src/model/objectbase.cpp
    src/model/types.cpp
    src/model/xrcfilter.cpp
    src/rad/codeeditor/codeeditor.cpp
    src/rad/cpppanel/cpppanel.cpp
    src/rad/dataobject/dataobject.cpp
    src/rad/designer/innerframe.cpp
    src/rad/designer/menubar.cpp
#   src/rad/designer/resizablepanel.cpp TODO: ???
    src/rad/designer/visualeditor.cpp
    src/rad/designer/visualobj.cpp
    src/rad/geninheritclass/geninhertclass.cpp
    src/rad/geninheritclass/geninhertclass_gui.cpp
    src/rad/inspector/advprops.cpp
    src/rad/inspector/objinspect.cpp
    src/rad/luapanel/luapanel.cpp
    src/rad/objecttree/objecttree.cpp
    src/rad/phppanel/phppanel.cpp
    src/rad/pythonpanel/pythonpanel.cpp
    src/rad/xrcpanel/xrcpanel.cpp
    src/rad/xrcpreview/xrcpreview.cpp
    src/rad/about.cpp
    src/rad/appdata.cpp
    src/rad/auitabart.cpp
    src/rad/bitmaps.cpp
    src/rad/cmdproc.cpp
    src/rad/customkeys.cpp
    src/rad/event.cpp
    src/rad/genericpanel.cpp
    src/rad/mainframe.cpp
    src/rad/manager.cpp
    src/rad/menueditor.cpp
    src/rad/palette.cpp
    src/rad/title.cpp
    src/utils/annoyingdialog.cpp
    src/utils/filetocarray.cpp
    src/utils/ipc.cpp
    src/utils/m_wxweaver.cpp
    src/utils/stringutils.cpp
    src/utils/typeconv.cpp
    src/maingui.cpp
)
if(APPLE)
    add_executable(${CMAKE_PROJECT_NAME}
        MACOSX_BUNDLE
        ${wxWEAVER_INCLUDE_FILES}
        ${wxWEAVER_SOURCE_FILES}
        ${wxWEAVER_RESOURCE_FILES}
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
    "src"
    "sdk/plugin_interface"
)
target_link_libraries(${CMAKE_PROJECT_NAME}
    ${wxWidgets_LIBRARIES}
    wxweaver::ticpp
    wxweaver::plugin_interface
)
if(NOT WIN32)
    target_link_libraries(${CMAKE_PROJECT_NAME} dl)
endif()

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
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/linux/icons"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/application"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/wxweaver"
    )
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/COPYING"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/wxweaver"
    )
endif()
