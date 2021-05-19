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

#include <wx/aui/framemanager.h>
#include <wx/aui/dockart.h>

class DockArt : public wxAuiDefaultDockArt {
public:
    DockArt();

    void DrawCaption(wxDC& dc, wxWindow* window, const wxString& text,
                     const wxRect& rect, wxAuiPaneInfo& pane);

    void DrawButton(wxDC& dc, wxWindow* window, int button, int button_state,
                    const wxRect& _rect, wxAuiPaneInfo& pane);

    void UpdateColoursFromSystem();

private:
    wxString EllipsizeText(wxDC& dc, const wxString& text, int maxSize);
    void InitBitmaps();
};
