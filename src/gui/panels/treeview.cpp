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
#include "gui/panels/treeview.h"

#include "rtti/objectbase.h"
#include "appdata.h"
#include "gui/bitmaps.h"
#include "gui/dialogs/menueditor.h"
#include "event.h"

#include <wx/imaglist.h>

#if 0
BEGIN_EVENT_TABLE(ObjectTree, wxPanel)
EVT_TREE_SEL_CHANGED(wxID_ANY, ObjectTree::OnSelChanged)
EVT_TREE_ITEM_RIGHT_CLICK(wxID_ANY, ObjectTree::OnRightClick)
EVT_TREE_BEGIN_DRAG(wxID_ANY, ObjectTree::OnBeginDrag)
EVT_TREE_END_DRAG(wxID_ANY, ObjectTree::OnEndDrag)
EVT_TREE_KEY_DOWN(wxID_ANY, ObjectTree::OnKeyDown)
EVT_WVR_PROJECT_LOADED(ObjectTree::OnProjectLoaded)
EVT_WVR_PROJECT_SAVED(ObjectTree::OnProjectSaved)
EVT_WVR_OBJECT_CREATED(ObjectTree::OnObjectCreated)
EVT_WVR_OBJECT_REMOVED(ObjectTree::OnObjectRemoved)
EVT_WVR_PROPERTY_MODIFIED(ObjectTree::OnPropertyModified)
EVT_WVR_PROJECT_REFRESH(ObjectTree::OnProjectRefresh)
END_EVENT_TABLE()
#endif

ObjectTree::ObjectTree(wxWindow* parent, int id)
    : wxPanel(parent, id)
{
    AppData()->AddHandler(this->GetEventHandler());
    m_tcObjects = new wxTreeCtrl(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT | wxTR_DEFAULT_STYLE | wxSIMPLE_BORDER);

    wxBoxSizer* sizer_1 = new wxBoxSizer(wxVERTICAL);
    sizer_1->Add(m_tcObjects, 1, wxEXPAND, 0);
    SetAutoLayout(true);
    SetSizer(sizer_1);
    sizer_1->Fit(this);
    sizer_1->SetSizeHints(this);

    Bind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &ObjectTree::OnExpansionChange, this);
    Bind(wxEVT_COMMAND_TREE_ITEM_COLLAPSED, &ObjectTree::OnExpansionChange, this);
    Bind(wxEVT_COMMAND_TREE_SEL_CHANGED, &ObjectTree::OnSelChanged, this);
    Bind(wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, &ObjectTree::OnRightClick, this);
    Bind(wxEVT_COMMAND_TREE_BEGIN_DRAG, &ObjectTree::OnBeginDrag, this);
    Bind(wxEVT_COMMAND_TREE_END_DRAG, &ObjectTree::OnEndDrag, this);
    Bind(wxEVT_COMMAND_TREE_KEY_DOWN, &ObjectTree::OnKeyDown, this);

    Bind(wxEVT_WVR_OBJECT_EXPANDED, &ObjectTree::OnObjectExpanded, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &ObjectTree::OnObjectSelected, this);
    Bind(wxEVT_WVR_PROJECT_LOADED, &ObjectTree::OnProjectLoaded, this);
    Bind(wxEVT_WVR_PROJECT_SAVED, &ObjectTree::OnProjectSaved, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &ObjectTree::OnObjectCreated, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &ObjectTree::OnObjectRemoved, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &ObjectTree::OnPropertyModified, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &ObjectTree::OnProjectRefresh, this);

    m_altKeyIsDown = false;
}

void ObjectTree::OnKeyDown(wxTreeEvent& event)
{
#ifdef __WXGTK__
    if (event.GetKeyEvent().AltDown() && event.GetKeyCode() != WXK_ALT) {
        switch (event.GetKeyCode()) {
        case WXK_UP:
            AppData()->MovePosition(
                GetObjectFromTreeItem(m_tcObjects->GetSelection()), false);
            return;
        case WXK_DOWN:
            AppData()->MovePosition(
                GetObjectFromTreeItem(m_tcObjects->GetSelection()), true);
            return;
        case WXK_RIGHT:
            AppData()->MoveHierarchy(
                GetObjectFromTreeItem(m_tcObjects->GetSelection()), false);
            return;
        case WXK_LEFT:
            AppData()->MoveHierarchy(
                GetObjectFromTreeItem(m_tcObjects->GetSelection()), true);
            return;
        }
    }
#endif
    event.Skip();
}

ObjectTree::~ObjectTree()
{
    AppData()->RemoveHandler(this->GetEventHandler());
}

PObjectBase ObjectTree::GetObjectFromTreeItem(wxTreeItemId item)
{
    if (item.IsOk()) {
        wxTreeItemData* itemData = m_tcObjects->GetItemData(item);
        if (itemData) {
            PObjectBase obj(((ObjectTreeItemData*)itemData)->GetObject());
            return obj;
        }
    }
    return PObjectBase((ObjectBase*)nullptr);
}

void ObjectTree::RebuildTree()
{
    m_tcObjects->Freeze();

    Unbind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &ObjectTree::OnExpansionChange, this);
    Unbind(wxEVT_COMMAND_TREE_ITEM_COLLAPSED, &ObjectTree::OnExpansionChange, this);

    PObjectBase project = AppData()->GetProjectData();

    // Clear the old tree and map
    m_tcObjects->DeleteAllItems();
    m_map.clear();

    if (project) {
        wxTreeItemId dummy;
        AddChildren(project, dummy, true);

        // Expand items that were previously expanded
        RestoreItemStatus(project);
    }
    Bind(wxEVT_COMMAND_TREE_ITEM_COLLAPSED, &ObjectTree::OnExpansionChange, this);
    Bind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &ObjectTree::OnExpansionChange, this);

    m_tcObjects->Thaw();
}

void ObjectTree::OnSelChanged(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    if (!id.IsOk())
        return;

    // Make selected items bold
    wxTreeItemId oldId = event.GetOldItem();
    if (oldId.IsOk())
        m_tcObjects->SetItemBold(oldId, false);

    m_tcObjects->SetItemBold(id);

    wxTreeItemData* itemData = m_tcObjects->GetItemData(id);
    if (itemData) {
        PObjectBase obj(((ObjectTreeItemData*)itemData)->GetObject());
        assert(obj);
        Unbind(wxEVT_WVR_OBJECT_SELECTED, &ObjectTree::OnObjectSelected, this);
        AppData()->SelectObject(obj);
        Bind(wxEVT_WVR_OBJECT_SELECTED, &ObjectTree::OnObjectSelected, this);
    }
}

void ObjectTree::OnRightClick(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    wxTreeItemData* itemData = m_tcObjects->GetItemData(id);
    if (itemData) {
        PObjectBase obj(((ObjectTreeItemData*)itemData)->GetObject());
        assert(obj);
        wxMenu* menu = new ItemPopupMenu(obj);
        wxPoint pos = event.GetPoint();
        menu->UpdateUI(menu);
        PopupMenu(menu, pos.x, pos.y);
    }
}

void ObjectTree::OnBeginDrag(wxTreeEvent& event)
{
    // need to explicitly allow drag
    if (event.GetItem() == m_tcObjects->GetRootItem())
        return;

    m_draggedItem = event.GetItem();
    event.Allow();
}

void ObjectTree::OnEndDrag(wxTreeEvent& event)
{
    bool copy = ::wxGetKeyState(WXK_CONTROL);

    wxTreeItemId itemSrc = m_draggedItem,
                 itemDst = event.GetItem();
    m_draggedItem = (wxTreeItemId)0l; // TODO: ???

    // ensure that itemDst is not itemSrc or a child of itemSrc
    wxTreeItemId item = itemDst;
    while (item.IsOk()) {
        if (item == itemSrc)
            return;

        item = m_tcObjects->GetItemParent(item);
    }
    PObjectBase objSrc = GetObjectFromTreeItem(itemSrc);
    if (!objSrc)
        return;

    PObjectBase objDst = GetObjectFromTreeItem(itemDst);
    if (!objDst)
        return;

    // backup clipboard
    PObjectBase clipboard = AppData()->GetClipboardObject();

    // set object to clipboard
    if (copy)
        AppData()->CopyObject(objSrc);
    else
        AppData()->CutObject(objSrc);

    if (!AppData()->PasteObject(objDst) && !copy)
        AppData()->Undo();

    AppData()->SetClipboardObject(clipboard);
}

void ObjectTree::OnExpansionChange(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    wxTreeItemData* itemData = m_tcObjects->GetItemData(id);
    if (itemData) {
        PObjectBase obj(((ObjectTreeItemData*)itemData)->GetObject());
        assert(obj);

        Unbind(wxEVT_WVR_OBJECT_EXPANDED, &ObjectTree::OnObjectExpanded, this);
        Unbind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &ObjectTree::OnExpansionChange, this);

        AppData()->ExpandObject(obj, m_tcObjects->IsExpanded(id));

        Bind(wxEVT_WVR_OBJECT_EXPANDED, &ObjectTree::OnObjectExpanded, this);
        Bind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &ObjectTree::OnExpansionChange, this);
    }
}

void ObjectTree::AddChildren(PObjectBase obj, wxTreeItemId& parent, bool isRoot)
{
    if (obj->GetObjectInfo()->GetType()->IsItem()) {
        if (obj->GetChildCount() > 0)
            AddChildren(obj->GetChild(0), parent);
        else {
            // Si hemos llegado aquí ha sido porque el arbol no está bien formado
            // y habrá que revisar cómo se ha creado.
            wxString msg;
            PObjectBase itemParent = obj->GetParent();
            assert(parent);
            msg = wxString::Format("Item without object as child of \'%s:%s\'",
                                   itemParent->GetPropertyAsString("name").c_str(),
                                   itemParent->GetClassName().c_str());
            wxLogError(msg);
        }
    } else {
        wxTreeItemId new_parent;
        ObjectTreeItemData* itemData = new ObjectTreeItemData(obj);

        if (isRoot)
            new_parent = m_tcObjects->AddRoot("", -1, -1, itemData);
        else {
            size_t pos = 0;

            PObjectBase parent_obj = obj->GetParent();
            // find a proper position where the added object should be displayed at
            if (parent_obj->GetObjectInfo()->GetType()->IsItem()) {
                parent_obj = parent_obj->GetParent();
                pos = parent_obj->GetChildPosition(obj->GetParent());
            } else
                pos = parent_obj->GetChildPosition(obj);

            // insert tree item to proper position
            if (pos > 0)
                new_parent = m_tcObjects->InsertItem(parent, pos, "", -1, -1, itemData);
            else
                new_parent = m_tcObjects->AppendItem(parent, "", -1, -1, itemData);
        }
        // Add the item to the map
        m_map.insert(ObjectItemMap::value_type(obj, new_parent));

        // Set the image
        int image_idx = GetImageIndex(obj->GetObjectInfo()->GetClassName());

        m_tcObjects->SetItemImage(new_parent, image_idx);

        // Set the name
        UpdateItem(new_parent, obj);

        // Add the rest of the children
        size_t count = obj->GetChildCount();
        for (size_t i = 0; i < count; i++) {
            PObjectBase child = obj->GetChild(i);
            AddChildren(child, new_parent);
        }
    }
}

int ObjectTree::GetImageIndex(wxString name)
{
    int index = 0; //default icon
    IconIndexMap::iterator it = m_iconIdx.find(name);
    if (it != m_iconIdx.end())
        index = it->second;

    return index;
}

void ObjectTree::UpdateItem(wxTreeItemId id, PObjectBase obj)
{
    // mostramos el nombre
    wxString className(obj->GetClassName());
    PProperty prop = obj->GetProperty("name");
    wxString objName;
    if (prop)
        objName = prop->GetValueAsString();

    wxString text = objName + " : " + className;
    m_tcObjects->SetItemText(id, text); // actualizamos el item
}

void ObjectTree::Create()
{
    // Cramos la lista de iconos obteniendo los iconos de los paquetes.
    size_t index = 0;
    m_iconList = new wxImageList(ICON_SIZE, ICON_SIZE);
    {
        wxBitmap icon = AppBitmaps::GetBitmap("project", ICON_SIZE);
        m_iconList->Add(icon);
        m_iconIdx.insert(IconIndexMap::value_type("_default_", index++));
    }
    size_t pkg_count = AppData()->GetPackageCount();
    for (size_t i = 0; i < pkg_count; i++) {
        PObjectPackage pkg = AppData()->GetPackage(i);
        for (size_t j = 0; j < pkg->GetObjectCount(); j++) {
            wxString comp_name(pkg->GetObjectInfo(j)->GetClassName());
            m_iconList->Add(pkg->GetObjectInfo(j)->GetIconFile());
            m_iconIdx.insert(IconIndexMap::value_type(comp_name, index++));
        }
    }
    m_tcObjects->AssignImageList(m_iconList);
}

void ObjectTree::RestoreItemStatus(PObjectBase obj)
{
    ObjectItemMap::iterator item_it = m_map.find(obj);
    if (item_it != m_map.end()) {
        wxTreeItemId id = item_it->second;

        if (obj->GetExpanded())
            m_tcObjects->Expand(id);
#if 0
        else
            m_tcObjects->Collapse(id);
#endif
    }
    size_t i, count = obj->GetChildCount();
    for (i = 0; i < count; i++)
        RestoreItemStatus(obj->GetChild(i));
}

void ObjectTree::AddItem(PObjectBase item, PObjectBase parent)
{
    if (item && parent) {
        // find parent item displayed in the object tree
        while (parent && parent->GetObjectInfo()->GetType()->IsItem())
            parent = parent->GetParent();

        // add new item to the object tree
        ObjectItemMap::iterator it = m_map.find(parent);
        if ((it != m_map.end()) && it->second.IsOk())
            AddChildren(item, it->second, false);
    }
}

void ObjectTree::RemoveItem(PObjectBase item)
{
    // remove affected object tree items only
    ObjectItemMap::iterator it = m_map.find(item);
    if ((it != m_map.end()) && it->second.IsOk()) {
        m_tcObjects->Delete(it->second);
        // clear map records for all item's children
        ClearMap(it->first);
    }
}

void ObjectTree::ClearMap(PObjectBase obj)
{
    m_map.erase(obj);

    for (size_t i = 0; i < obj->GetChildCount(); i++)
        ClearMap(obj->GetChild(i));
}
// =============================================================================
//  wxWeaver Event Handlers
// =============================================================================
void ObjectTree::OnProjectLoaded(wxWeaverEvent&)
{
    RebuildTree();
}

void ObjectTree::OnProjectSaved(wxWeaverEvent&)
{
}

void ObjectTree::OnObjectExpanded(wxWeaverObjectEvent& event)
{
    PObjectBase obj = event.GetWvrObject();
    ObjectItemMap::iterator it = m_map.find(obj);
    if (it != m_map.end()) {
        if (m_tcObjects->IsExpanded(it->second) != obj->GetExpanded()) {
            if (obj->GetExpanded())
                m_tcObjects->Expand(it->second);
            else
                m_tcObjects->Collapse(it->second);
        }
    }
}

void ObjectTree::OnObjectSelected(wxWeaverObjectEvent& event)
{
    PObjectBase obj = event.GetWvrObject();

    // Find the tree item associated with the object and select it
    ObjectItemMap::iterator it = m_map.find(obj);
    if (it != m_map.end()) {
        // Ignore expand/collapse events
        Unbind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &ObjectTree::OnExpansionChange, this);
        Unbind(wxEVT_COMMAND_TREE_ITEM_COLLAPSED, &ObjectTree::OnExpansionChange, this);

        m_tcObjects->EnsureVisible(it->second);
        m_tcObjects->SelectItem(it->second);

        // Restore event handling
        Bind(wxEVT_COMMAND_TREE_ITEM_EXPANDED, &ObjectTree::OnExpansionChange, this);
        Bind(wxEVT_COMMAND_TREE_ITEM_COLLAPSED, &ObjectTree::OnExpansionChange, this);
    } else {
        wxLogError(
            "There is no tree item associated with this object.\n\tClass: %s\n\tName: %s",
            obj->GetClassName().c_str(), obj->GetPropertyAsString("name").c_str());
    }
}

void ObjectTree::OnObjectCreated(wxWeaverObjectEvent& event)
{
#if 0
    RebuildTree();
#endif
    if (event.GetWvrObject())
        AddItem(event.GetWvrObject(), event.GetWvrObject()->GetParent());
}

void ObjectTree::OnObjectRemoved(wxWeaverObjectEvent& event)
{
#if 0
    RebuildTree();
#endif
    RemoveItem(event.GetWvrObject());
}

void ObjectTree::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    PProperty prop = event.GetWvrProperty();
    if (prop->GetName() == "name") {
        ObjectItemMap::iterator it = m_map.find(prop->GetObject());
        if (it != m_map.end())
            UpdateItem(it->second, it->first);
    }
}

void ObjectTree::OnProjectRefresh(wxWeaverEvent&)
{
    RebuildTree();
}

ObjectTreeItemData::ObjectTreeItemData(PObjectBase obj)
    : m_object(obj)
{
}
// TODO: Use wxID_XXX
enum {
    MENU_MOVE_UP = wxID_HIGHEST + 2000,
    MENU_MOVE_DOWN,
    MENU_MOVE_RIGHT,
    MENU_MOVE_LEFT,
    MENU_CUT,
    MENU_PASTE,
    MENU_EDIT_MENUS,
    MENU_COPY,
    MENU_MOVE_NEW_BOXSIZER,
    MENU_DELETE,
};

ItemPopupMenu::ItemPopupMenu(PObjectBase obj)
    : m_object(obj)
{
    Append(MENU_CUT, _("Cut\tCtrl+X"));
    Append(MENU_COPY, _("Copy\tCtrl+C"));
    Append(MENU_PASTE, _("Paste\tCtrl+V"));
    AppendSeparator();
    Append(MENU_DELETE, _("Delete\tCtrl+D"));
    AppendSeparator();
    Append(MENU_MOVE_UP, _("Move Up\tAlt+Up"));
    Append(MENU_MOVE_DOWN, _("Move Down\tAlt+Down"));
    Append(MENU_MOVE_LEFT, _("Move Left\tAlt+Left"));
    Append(MENU_MOVE_RIGHT, _("Move Right\tAlt+Right"));
    AppendSeparator();
    Append(MENU_MOVE_NEW_BOXSIZER, _("Move into a new wxBoxSizer"));
    AppendSeparator();
    Append(MENU_EDIT_MENUS, _("Menu Editor..."));

    Bind(wxEVT_MENU, &ItemPopupMenu::OnMenuEvent, this);
    Bind(wxEVT_UPDATE_UI, &ItemPopupMenu::OnUpdateEvent, this);
}

void ItemPopupMenu::OnMenuEvent(wxCommandEvent& event)
{
    int id = event.GetId();

    switch (id) {
    case MENU_CUT:
        AppData()->CutObject(m_object);
        break;
    case MENU_COPY:
        AppData()->CopyObject(m_object);
        break;
    case MENU_PASTE:
        AppData()->PasteObject(m_object);
        break;
    case MENU_DELETE:
        AppData()->RemoveObject(m_object);
        break;
    case MENU_MOVE_UP:
        AppData()->MovePosition(m_object, false);
        break;
    case MENU_MOVE_DOWN:
        AppData()->MovePosition(m_object, true);
        break;
    case MENU_MOVE_RIGHT:
        AppData()->MoveHierarchy(m_object, false);
        break;
    case MENU_MOVE_LEFT:
        AppData()->MoveHierarchy(m_object, true);
        break;
    case MENU_MOVE_NEW_BOXSIZER:
        AppData()->CreateBoxSizerWithObject(m_object);
        break;
    case MENU_EDIT_MENUS: {
        PObjectBase obj = m_object;
        if (obj && (obj->GetClassName() == "wxMenuBar" || obj->GetClassName() == "Frame")) {
            MenuEditor menuEditor;
            if (obj->GetClassName() == "Frame") {
                bool found = false;
                PObjectBase menubar;
                for (size_t i = 0; i < obj->GetChildCount() && !found; i++) {
                    menubar = obj->GetChild(i);
                    found = menubar->GetClassName() == "wxMenuBar";
                }
                if (found)
                    obj = menubar;
            }
            if (obj->GetClassName() == "wxMenuBar")
                menuEditor.Populate(obj);
            if (menuEditor.ShowModal() == wxID_OK) {
                if (obj->GetClassName() == "wxMenuBar") {
                    PObjectBase menubar
                        = menuEditor.GetMenubar(AppData()->GetObjectDatabase());

                    while (obj->GetChildCount() > 0) {
                        PObjectBase child = obj->GetChild(0);
                        obj->RemoveChild(0);
                        child->SetParent(PObjectBase());
                    }
                    for (size_t i = 0; i < menubar->GetChildCount(); i++) {
                        PObjectBase child = menubar->GetChild(i);
                        AppData()->InsertObject(child, obj);
                    }
                } else
                    AppData()->InsertObject(
                        menuEditor.GetMenubar(AppData()->GetObjectDatabase()),
                        AppData()->GetSelectedForm());
            }
        }
    } break;
    default:
        break;
    }
}

void ItemPopupMenu::OnUpdateEvent(wxUpdateUIEvent& e)
{
    switch (e.GetId()) {
    case MENU_EDIT_MENUS:
        e.Enable(
            m_object
            && (m_object->GetClassName() == "wxMenuBar"
                || m_object->GetClassName() == "Frame"));
        break;
    case MENU_CUT:
    case MENU_COPY:
    case MENU_DELETE:
    case MENU_MOVE_UP:
    case MENU_MOVE_DOWN:
    case MENU_MOVE_LEFT:
    case MENU_MOVE_RIGHT:
    case MENU_MOVE_NEW_BOXSIZER:
        e.Enable(AppData()->CanCopyObject());
        break;
    case MENU_PASTE:
        e.Enable(AppData()->CanPasteObject());
        break;
    }
}
