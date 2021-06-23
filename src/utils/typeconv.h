/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
    Copyright (C) 2021 Andrea Zanellato <redtid3@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#pragma once

#include "rtti/types.h"

#include <fontcontainer.h>

namespace TypeConv {

wxPoint StringToPoint(const wxString& str);
bool StringToPoint(const wxString& str, wxPoint* point);
wxSize StringToSize(const wxString& str);

wxString PointToString(const wxPoint& point);
wxString SizeToString(const wxSize& size);

int BitlistToInt(const wxString& str);
int GetMacroValue(const wxString& str);
int StringToInt(const wxString& str);

bool FlagSet(const wxString& flag, const wxString& currentValue);
wxString ClearFlag(const wxString& flag, const wxString& currentValue);
wxString SetFlag(const wxString& flag, const wxString& currentValue);

wxBitmap StringToBitmap(const wxString& filename);

wxFontContainer StringToFont(const wxString& str);
wxString FontToString(const wxFontContainer& font);
wxString FontFamilyToString(wxFontFamily family);
wxString FontStyleToString(wxFontStyle style);
wxString FontWeightToString(wxFontWeight weight);

wxColour StringToColour(const wxString& str);
wxSystemColour StringToSystemColour(const wxString& str);
wxString ColourToString(const wxColour& colour);
wxString SystemColourToString(long colour);

bool StringToBool(const wxString& str);
wxString BoolToString(bool val);

wxArrayString StringToArrayString(const wxString& str);
wxString ArrayStringToString(const wxArrayString& arrayStr);

void ParseBitmapWithResource(const wxString& value, wxString* image,
                             wxString* source, wxSize* icoSize);

/** @internal Used to import old projects.
*/
wxArrayString OldStringToArrayString(const wxString& str);

wxString ReplaceSynonymous(const wxString& bitlist);

void SplitFileSystemURL(const wxString& url, wxString* protocol, wxString* path,
                        wxString* anchor);

// Obtiene la ruta absoluta de un archivo
wxString MakeAbsolutePath(const wxString& filename, const wxString& basePath);
wxString MakeAbsoluteURL(const wxString& url, const wxString& basePath);

// Obtiene la ruta relativa de un archivo
wxString MakeRelativePath(const wxString& filename, const wxString& basePath);
wxString MakeRelativeURL(const wxString& url, const wxString& basePath);

// dada una cadena de caracteres obtiene otra transformando los caracteres
// especiales denotados al estilo C ('\n' '\\' '\t')
wxString StringToText(const wxString& str);
wxString TextToString(const wxString& str);

double StringToFloat(const wxString& str);
wxString FloatToString(const double& val);
} // namespace TypeConv
/*
    I don't really like having to use global variables or singletons
    but until we find another more elegant design we will continue with this.
    TODO: include it in GlobalApplicationData
*/
class MacroDictionary;
typedef MacroDictionary* PMacroDictionary;

class MacroDictionary {
public:
    static PMacroDictionary GetInstance();
    static void Destroy();
    bool SearchMacro(wxString name, int* result);
    void AddMacro(wxString name, int value);
    void AddSynonymous(wxString synName, wxString name);
    bool SearchSynonymous(wxString synName, wxString& result);

private:
    MacroDictionary();
    static PMacroDictionary s_instance;

    typedef std::map<wxString, int> MacroMap;
    MacroMap m_map;

    typedef std::map<wxString, wxString> SynMap;
    SynMap m_synMap;
};
