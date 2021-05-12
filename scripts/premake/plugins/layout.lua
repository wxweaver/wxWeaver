-----------------------------------------------------------------------------
--  Name:        layout.lua
--  Purpose:     Layout controls plugin project build script.
--  Author:      Andrea Zanellato
--  Modified by:
--  Created:     21/10/2011
--  Copyright:   (c) 2011 wxFormBuilder Team
--               (c) 2021 wxWeaver Team
--  Licence:     GNU General Public License Version 2
-----------------------------------------------------------------------------
project "layout-components-plugin"
    kind                "SharedLib"
    targetname          "layout"
    files               {"../../../src/plugins/layout/layout.cpp"}
    includedirs
    {
        "../../../external/ticpp", "../../../sdk/plugin_interface"
    }
    defines             {"BUILD_DLL", "TIXML_USE_TICPP"}
    links               {"plugin-interface", "TiCPP"}

    local libs = "std,richtext,propgrid,stc,ribbon,aui"

    configuration "not vs*"
        buildoptions    "-std=c++17"

    configuration "vs*"
        buildoptions    "/std:c++17"

    configuration "not windows"
        targetdir       "../../../lib/wxweaver"

    configuration "windows"
        targetprefix    "lib"
        targetdir       "../../../plugins/layout"

    configuration "Debug"
        wx_config       {Debug="yes", Libs=libs}
        targetsuffix    (DebugSuffix)

    configuration "Release"
        wx_config       {Libs=libs}

    configuration {"not vs*", "Release"}
        buildoptions    {"-fno-strict-aliasing"}
