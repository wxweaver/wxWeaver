-----------------------------------------------------------------------------
--  Name:        utilities.lua
--  Purpose:     Directories creation for local builds, Linux install script.
--               Based on original Premake 3 script
--  Author:      Andrea Zanellato
--  Modified by:
--  Created:
--  Copyright:   (c) 2011 wxFormBuilder Team
--               (c) 2021 wxWeaver Team
--  Licence:     GNU General Public License Version 2
-----------------------------------------------------------------------------
function CreateShareReadme( filename )
    local text =
    "This directory and the symlink it contains were created automatically by premake\n" ..
    "to facilitate execution of wxWeaver prior to installation on Unix platforms.\n" ..
    "On Unix, wxWeaver expects to be executed from a directory named \"output\",\n" ..
    "which is next to a directory named \"share\". The \"share\" directory should have a\n" ..
    "subdirectory named \"wxweaver\", which contains the configuration files."

    if ( not os.isfile( filename )  ) then
        local fo = io.open( filename, "w" )
        fo:write( text )
        fo:close()
    end
end

function CreateSymlink( pathToLink, symLink )
    os.execute( "ln -s -f -n " .. pathToLink .. " " .. symLink  )
end

if ( not os.is("windows") ) then
    newoption {
        trigger     = "skip-symlink",
        description = "Do not create the symlink output/share/wxweaver"
    }
    if ( not _OPTIONS["skip-symlink"] ) then
        print( "Running pre-build file creation steps..." )
        -- Create a 'share' directory.
        -- HACK: make sure the warning isn't shown on the console.
        os.execute( "mkdir ../../output/share >/dev/null 2>/dev/null" )
        CreateShareReadme( "../../output/share/README.txt" )
        CreateSymlink( "../../output", "../../output/share/wxweaver" )
    end
end

if ( os.is("linux") ) then
    newoption {
        trigger     = "install",
        description = "Copy the application to the specified path"
    }
    function doinstall(cmd, arg)
        if ( not arg ) then
            error("You must specify an install location")
        end

        os.execute( "../../install/linux/wxweaver_export.sh " .. arg )
    end
end
