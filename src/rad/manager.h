/*
    wxWeaver - A GUI Designer Editor for wxWidgets.
    Copyright (C) 2005 José Antonio Hurtado
    Copyright (C) 2005 Juan Antonio Ortega
    Copyright (C) 2005 Ryan Mulder (as wxFormBuilder)
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

#include "utils/defs.h"
#include <component.h>

class VisualEditor;
class ObjectBase;

class wxWeaverManager : public IManager {
public:
    wxWeaverManager();
    void SetVisualEditor(VisualEditor* visualEdit);
    size_t GetChildCount(wxObject* wxobject) override;
    wxObject* GetChild(wxObject* wxobject, size_t childIndex) override;
    wxObject* GetParent(wxObject* wxobject) override;
    IObject* GetIParent(wxObject* wxobject) override;
    IObject* GetIObject(wxObject* wxobject) override;
    wxObject* GetWxObject(PObjectBase obj);
    wxNoObject* NewNoObject() override;

    void ModifyProperty(wxObject* wxobject, wxString property, wxString value,
                        bool allowUndo = true) override;

    /** Returns @true if selection changed, @false if already selected
    */
    bool SelectObject(wxObject* wxobject) override;

private:
    VisualEditor* m_visualEdit;
};
