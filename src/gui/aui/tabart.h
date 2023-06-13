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

#include <wx/aui/tabart.h>

class AuiTabArt : public wxAuiGenericTabArt {
public:
    AuiTabArt() { UpdateColoursFromSystem(); }

    AuiTabArt* Clone() { return new AuiTabArt(*this); }

    void UpdateColoursFromSystem();

    void DrawTab(wxDC& dc,
                 wxWindow* wnd,
                 const wxAuiNotebookPage& page,
                 const wxRect& in_rect,
                 int close_button_state,
                 wxRect* out_tab_rect,
                 wxRect* out_button_rect,
                 int* x_extent);

    wxSize GetTabSize(wxDC& dc,
                      wxWindow* wnd,
                      const wxString& caption,
                      const wxBitmapBundle &bitmap,
                      bool active,
                      int close_button_state,
                      int* x_extent);
};
