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

#include "model/database.h"
#include <wx/aui/auibar.h>
#include <wx/aui/auibook.h>
#include <wx/spinbutt.h>

#include <set>

class Palette : public wxPanel {
public:
    Palette(wxWindow* parent, int id);

    void Create();
    void SaveSettings();

    void OnSpinUp(wxSpinEvent& e);
    void OnSpinDown(wxSpinEvent& e);
    void OnButtonClick(wxCommandEvent& event);

private:
    void PopulateToolbar(PObjectPackage pkg, wxAuiToolBar* toolbar);

    typedef std::vector<wxAuiToolBar*> ToolbarVector;
    ToolbarVector m_toolbars;
    wxAuiNotebook* m_notebook;
    std::set<wxString> m_pkgNames;
};
#if 0
class PaletteButton : public wxBitmapButton {
public:
    PaletteButton(wxWindow* parent, const wxBitmap& bitmap, wxString& name);
    void OnButtonClick(wxCommandEvent& event);

private:
    wxString m_name;
};

class ToolPanel : public wxPanel, public DataObserver {
public:
    ToolPanel(wxWindow* parent, int id);
    void OnSaveFile(wxCommandEvent& event);
};

class PaletteButtonEventHandler : public wxEvtHandler {
public:
    PaletteButtonEventHandler(wxString name, DataObservable* data);
    void OnButtonClick(wxCommandEvent& event);

private:
    wxString m_name;
    DataObservable* m_data;
};
#endif
