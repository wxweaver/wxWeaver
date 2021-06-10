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

#include <wx/menu.h>
#include <wx/panel.h>

#include <vector>

class wxBoxSizer;
class wxStaticText;

class Menubar : public wxPanel {
public:
    Menubar();
    Menubar(wxWindow* parent, int id,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = 0,
            const wxString& name = "wxwMenubar");
    ~Menubar() override;

    void AppendMenu(const wxString& name, wxMenu* menu);
    wxMenu* GetMenu(int index);
    wxMenu* Remove(int index);
    size_t GetMenuCount() const;

private:
    typedef std::vector<wxMenu*> MenuVector;
    MenuVector m_menus;
    wxBoxSizer* m_sizer;
};

class MenuEvtHandler : public wxEvtHandler {
public:
    MenuEvtHandler(wxStaticText* staticText, wxMenu* menu);
    void OnMouseEvent(wxMouseEvent& event);

private:
    wxStaticText* m_label;
    wxMenu* m_menu;
};
