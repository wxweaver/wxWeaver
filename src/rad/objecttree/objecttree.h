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

#include "utils/defs.h"
#if 0
#include "rad/customkeys.h"
#endif
#include <wx/menu.h>
#include <wx/treectrl.h>

class wxWeaverEvent;
class wxWeaverPropertyEvent;
class wxWeaverObjectEvent;

class ObjectTree : public wxPanel {
public:
    ObjectTree(wxWindow* parent, int id);
    ~ObjectTree() override;

    void Create();

    void OnSelChanged(wxTreeEvent& event);
    void OnRightClick(wxTreeEvent& event);
    void OnBeginDrag(wxTreeEvent& event);
    void OnEndDrag(wxTreeEvent& event);
    void OnExpansionChange(wxTreeEvent& event);

    void OnProjectLoaded(wxWeaverEvent& event);
    void OnProjectSaved(wxWeaverEvent& event);
    void OnObjectExpanded(wxWeaverObjectEvent& event);
    void OnObjectSelected(wxWeaverObjectEvent& event);
    void OnObjectCreated(wxWeaverObjectEvent& event);
    void OnObjectRemoved(wxWeaverObjectEvent& event);
    void OnPropertyModified(wxWeaverPropertyEvent& event);
    void OnProjectRefresh(wxWeaverEvent& event);
    void OnKeyDown(wxTreeEvent& event);
#if 0
    void AddCustomKeysHandler(CustomKeysEvtHandler* h) { m_tcObjects->PushEventHandler(h); }
#endif

private:
    void RebuildTree();
    void AddChildren(PObjectBase child, wxTreeItemId& parent, bool is_root = false);
    void UpdateItem(wxTreeItemId id, PObjectBase obj);
    void RestoreItemStatus(PObjectBase obj);
    void AddItem(PObjectBase item, PObjectBase parent);
    void RemoveItem(PObjectBase item);
    void ClearMap(PObjectBase obj);
    int GetImageIndex(wxString type);

    PObjectBase GetObjectFromTreeItem(wxTreeItemId item);

    typedef std::map<PObjectBase, wxTreeItemId> ObjectItemMap;
    ObjectItemMap m_map;

    typedef std::map<wxString, int> IconIndexMap;
    IconIndexMap m_iconIdx;

    wxImageList* m_iconList;
    wxTreeItemId m_draggedItem;
    wxTreeCtrl* m_tcObjects;

    bool m_altKeyIsDown;
};

/** Gracias a que podemos asociar un objeto a cada item, esta clase nos va
    a facilitar obtener el objeto (ObjectBase) asociado a un item para
    seleccionarlo pinchando en el item.
*/
class ObjectTreeItemData : public wxTreeItemData {
public:
    ObjectTreeItemData(PObjectBase obj);
    PObjectBase GetObject() { return m_object; }

private:
    PObjectBase m_object;
};

/** Menu popup asociado a cada item del arbol.

    Este objeto ejecuta los comandos incluidos en el menu referentes al objeto
    seleccionado.
*/
class ItemPopupMenu : public wxMenu {
public:
    ItemPopupMenu(PObjectBase obj);

    void OnUpdateEvent(wxUpdateUIEvent& e);
    void OnMenuEvent(wxCommandEvent& event);

private:
    PObjectBase m_object;
};
