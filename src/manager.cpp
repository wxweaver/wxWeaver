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
#include "manager.h"

#include "model/objectbase.h"
#include "appdata.h"
#include "gui/panels/designer/visualeditor.h"

#define CHECK_NULLPTR(THING, THING_NAME, RETURN)                               \
    if (!THING) {                                                              \
        wxLogError("%s is nullptr! <%s,%i>", THING_NAME, __TFILE__, __LINE__); \
        return RETURN;                                                         \
    }

#define CHECK_VISUAL_EDITOR(RETURN) \
    CHECK_NULLPTR(m_visualEdit, "VisualEditor", RETURN)

#define CHECK_WX_OBJECT(RETURN) \
    CHECK_NULLPTR(wxobject, "wxObject", RETURN)

#define CHECK_OBJECT_BASE(RETURN) \
    CHECK_NULLPTR(obj, "ObjectBase", RETURN)

// Classes to unset flags in VisualEditor during the destructor
// this prevents forgetting to unset the flag
class FlagFlipper {
public:
    FlagFlipper(VisualEditor* visualEdit, void (VisualEditor::*flagFunction)(bool))
        : m_visualEditor(visualEdit)
        , m_flagFunction(flagFunction)
    {
        (m_visualEditor->*m_flagFunction)(true);
    }

    ~FlagFlipper()
    {
        (m_visualEditor->*m_flagFunction)(false);
    }

private:
    VisualEditor* m_visualEditor;
    void (VisualEditor::*m_flagFunction)(bool);
};

wxWeaverManager::wxWeaverManager()
    : m_visualEdit(nullptr)
{
}

void wxWeaverManager::SetVisualEditor(VisualEditor* visualEdit)
{
    m_visualEdit = visualEdit;
}

IObject* wxWeaverManager::GetIObject(wxObject* wxobject)
{
    CHECK_VISUAL_EDITOR(nullptr)
    CHECK_WX_OBJECT(nullptr)
    PObjectBase obj = m_visualEdit->GetObjectBase(wxobject);
    CHECK_OBJECT_BASE(nullptr)
    return obj.get();
}

size_t wxWeaverManager::GetChildCount(wxObject* wxobject)
{
    CHECK_VISUAL_EDITOR(0)
    CHECK_WX_OBJECT(0)
    PObjectBase obj = m_visualEdit->GetObjectBase(wxobject);
    CHECK_OBJECT_BASE(0)
    return obj->GetChildCount();
}

wxObject* wxWeaverManager::GetChild(wxObject* wxobject, size_t childIndex)
{
    CHECK_VISUAL_EDITOR(nullptr)
    CHECK_WX_OBJECT(nullptr)
    PObjectBase obj = m_visualEdit->GetObjectBase(wxobject);
    CHECK_OBJECT_BASE(nullptr)
    if (childIndex >= obj->GetChildCount())
        return nullptr;

    return m_visualEdit->GetWxObject(obj->GetChild(childIndex));
}

IObject* wxWeaverManager::GetIParent(wxObject* wxobject)
{
    CHECK_VISUAL_EDITOR(nullptr)
    CHECK_WX_OBJECT(nullptr)
    PObjectBase obj = m_visualEdit->GetObjectBase(wxobject);
    CHECK_OBJECT_BASE(nullptr)
    return obj->GetParent().get();
}

wxObject* wxWeaverManager::GetParent(wxObject* wxobject)
{
    CHECK_VISUAL_EDITOR(nullptr)
    CHECK_WX_OBJECT(nullptr)
    PObjectBase obj = m_visualEdit->GetObjectBase(wxobject);
    CHECK_OBJECT_BASE(nullptr)
    return m_visualEdit->GetWxObject(obj->GetParent());
}

wxObject* wxWeaverManager::GetWxObject(PObjectBase obj)
{
    CHECK_OBJECT_BASE(nullptr)
    return m_visualEdit->GetWxObject(obj);
}

void wxWeaverManager::ModifyProperty(wxObject* wxobject, wxString property,
                                     wxString value, bool allowUndo)
{
    CHECK_VISUAL_EDITOR()
    // Prevent modified event in visual editor
    // no need to redraw when the change is happening in the editor!
    FlagFlipper stopModifiedEvent(m_visualEdit, &VisualEditor::PreventOnModified);
    CHECK_WX_OBJECT()
    PObjectBase obj = m_visualEdit->GetObjectBase(wxobject);
    CHECK_OBJECT_BASE()
    PProperty prop = obj->GetProperty(property);
    if (!prop) {
        wxLogError(
            "%s has no property named %s",
            obj->GetClassName().c_str(), property.c_str());
        return;
    }
    if (allowUndo)
        AppData()->ModifyProperty(prop, value);
    else
        prop->SetValue(value);
}

bool wxWeaverManager::SelectObject(wxObject* wxobject)
{
    CHECK_VISUAL_EDITOR(false)
    // Prevent loop of selection events
    FlagFlipper stopSelectedEvent(m_visualEdit, &VisualEditor::PreventOnSelected);
    CHECK_WX_OBJECT(false)
    PObjectBase obj = m_visualEdit->GetObjectBase(wxobject);
    CHECK_OBJECT_BASE(false)
    return AppData()->SelectObject(obj);
}

wxNoObject* wxWeaverManager::NewNoObject()
{
    return new wxNoObject;
}
