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
#include "settings.h"

#include <wx/config.h>
#include <wx/font.h>

namespace Default {
constexpr bool showIndentationGuides { true };
constexpr bool showEOL { false };
constexpr bool tabIndents { false };
constexpr bool useTabs { false };
constexpr bool localeEnabled { false };
constexpr int showWhiteSpace { 1 };
constexpr int tabsWidth { 4 };
constexpr int indentSize { 4 };
constexpr int caretWidth { 1 };
constexpr int fontSize { 8 };
constexpr int locale { 0 };
} // namespace Default

wxw::Preferences::Preferences()
{
    wxFont font(Default::fontSize, wxFONTFAMILY_MODERN, // Default font
                wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    wxConfigBase* config = wxConfigBase::Get();
    config->Read("/Editors/ShowIndentationGuides", &showIndentationGuides, Default::showIndentationGuides);
    config->Read("/Editors/ShowWhitespace", &showWhiteSpace, Default::showWhiteSpace);
    config->Read("/Editors/ShowEOL", &showEOL, Default::showEOL);
    config->Read("/Editors/TabIndents", &tabIndents, Default::tabIndents);
    config->Read("/Editors/UseTabs", &useTabs, Default::useTabs);
    config->Read("/Editors/TabsWidth", &tabsWidth, Default::tabsWidth);
    config->Read("/Editors/IndentSize", &indentSize, Default::indentSize);
    config->Read("/Editors/CaretWidth", &caretWidth, Default::caretWidth);
    config->Read("/Editors/FontFace", &fontFace, font.GetFaceName());
    config->Read("/Editors/FontSize", &fontSize, Default::fontSize);
    config->Read("/Locale/Enabled", &localeEnabled, Default::localeEnabled);
    config->Read("/Locale/Language", &localeSelected, Default::locale);

    if (showWhiteSpace < 0 || showWhiteSpace > 3)
        showWhiteSpace = Default::showWhiteSpace;

    if (tabsWidth < 1 || tabsWidth > 8)
        tabsWidth = Default::tabsWidth;

    if (indentSize < 1 || indentSize > 8)
        indentSize = Default::indentSize;

    if (caretWidth < 1 || caretWidth > 10)
        caretWidth = Default::caretWidth;

    if (fontSize < 4 || fontSize > 100)
        fontSize = Default::fontSize;

    if (localeSelected < 0 || localeSelected > 1)
        localeSelected = Default::locale;
}
