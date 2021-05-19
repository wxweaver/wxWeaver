/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
    Copyright (C) 2012-2021 Andrea Zanellato <redtid3@gmail.com>

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

#include <wx/colour.h>

class wxBitmap;
class wxDC;
class wxRect;
class wxString;

namespace wxw {
static bool IsDark(const wxColour& colour)
{
    int average = (colour.Red() + colour.Green() + colour.Blue()) / 3;
    if (average < 128)
        return true;

    return false;
}

void GlossyGradient(wxDC& dc, wxRect& rect, wxColour& topStart,
                    wxColour& bottomStart, wxColour& bottomEnd,
                    wxColour& colour, bool hover = false);

void Bitmap(wxDC& dc, const wxBitmap& bitmap, const wxRect& rect,
            const wxString& text);
} // namespace wxw
