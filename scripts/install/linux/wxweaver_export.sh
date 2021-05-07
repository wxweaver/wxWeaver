#!/bin/sh
#*****************************************************************************
#* Copyright (C) 2007 Ryan Mulder (as wxFormBuilder)
#* Copyright (C) 2007 Andrea Zanellato
#*
#* This program is free software; you can redistribute it and/or
#* modify it under the terms of the GNU General Public License
#* as published by the Free Software Foundation; either version 2
#* of the License, or (at your option) any later version.
#*
#* This program is distributed in the hope that it will be useful,
#* but WITHOUT ANY WARRANTY; without even the implied warranty of
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#* GNU General Public License for more details.
#*
#* You should have received a copy of the GNU General Public License
#* along with this program; if not, write to the Free Software
#* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#*
#*****************************************************************************
set -e

# Get output dir from caller
if [ ! -n "$1" ]
then
 echo "Please specify output directory."
 exit
else
 outputDir=$1
fi

# Ensure target directories exist  
if [ ! -d $outputDir ]
then
  mkdir $outputDir
  mkdir $outputDir/bin
  mkdir $outputDir/share
  mkdir $outputDir/share/wxweaver
fi

if [ ! -d $outputDir/bin ]
then
  mkdir $outputDir/bin
fi
  
if [ ! -d $outputDir/share ]
then
  mkdir $outputDir/share
  mkdir $outputDir/share/wxweaver
fi
  
if [ ! -d $outputDir/share/wxweaver ]
then
  mkdir $outputDir/share/wxweaver
fi

# copy ouput files to target directories  
cp -R --interactive output/* $outputDir/share/wxweaver/

# reorganize target directories
if [ -d $outputDir/bin ]
then
  mv -f $outputDir/share/wxweaver/bin/* $outputDir/bin
  rm -r $outputDir/share/wxweaver/bin
else
  mv -f $outputDir/share/wxweaver/bin $outputDir/
fi

if [ -d $outputDir/lib ]
then
  mv -f $outputDir/share/wxweaver/lib/* $outputDir/lib
  rm - r $outputDir/share/wxweaver/lib
else
 mv -f $outputDir/share/wxweaver/lib $outputDir/
fi

if [ -d $outputDir/share/wxweaver/share ]
then
 rm -r $outputDir/share/wxweaver/share
fi

exit

