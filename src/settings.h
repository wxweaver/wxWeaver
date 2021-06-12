/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
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

#include <wx/string.h>

namespace Default {
constexpr bool showIndentationGuides { true };
constexpr bool showEOL { false };
constexpr bool tabIndents { false };
constexpr bool useTabs { false };
constexpr int showWhiteSpace { 1 };
constexpr int tabsWidth { 4 };
constexpr int indentSize { 4 };
constexpr int caretWidth { 1 };
constexpr int fontSize { 8 };
} // namespace Default

struct PrefsEditor {
    PrefsEditor();

    void load();

    wxString fontFace;
    bool showIndentationGuides;
    bool showEOL;
    bool tabIndents;
    bool useTabs;
    int showWhiteSpace;
    int tabsWidth;
    int indentSize;
    int caretWidth;
    int fontSize;
};
