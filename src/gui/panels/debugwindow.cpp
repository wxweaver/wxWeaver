/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega (as wxFormBuilder)
    Copyright (C) 2011-2021 Jefferson González <jgmdev@gmail.com>
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
#include "debugwindow.h"

#include <wx/textctrl.h>
#include <wx/tokenzr.h>

DebugWindow::DebugWindow(wxWindow* parent)
    : wxTextCtrl(parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                 wxTE_MULTILINE | wxTE_READONLY | wxTE_LEFT)
{
    Bind(wxEVT_COMMAND_TEXT_UPDATED, &DebugWindow::OnTextUpdated, this);
}

void DebugWindow::OnTextUpdated(wxCommandEvent& event)
{
    wxString msg = event.GetString();

    wxArrayString lines = wxStringTokenize(msg, "\n");
    for (size_t i = 0; i < lines.GetCount(); i++) {
        if (lines.Item(i).Contains(_("Error: ")))
            SetDefaultStyle(wxTextAttr(wxColour(210, 0, 0)));
        else if (lines.Item(i).Contains(_("Warning: ")))
            SetDefaultStyle(wxTextAttr(wxColour(255, 150, 0)));
        else
            SetDefaultStyle(wxTextAttr(wxColour(0, 150, 0)));
    }
}
