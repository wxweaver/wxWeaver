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
#include "gui/dialogs/menueditor.h"
#include "model/objectbase.h"

namespace wxw {
constexpr int IDENTATION = 4;
enum {
    ID_ADDMENUITEM = wxID_HIGHEST + 1,
    ID_ADDSEPARATOR,
    ID_MENUDOWN,
    ID_MENULEFT,
    ID_MENURIGHT,
    ID_MENUUP,
    ID_REMOVEMENUITEM,
    ID_LABEL,
    ID_MODIFYMENUITEM,
};
} // namespace wxw
using namespace wxw;

#if 0
BEGIN_EVENT_TABLE(MenuEditor, wxDialog)
EVT_BUTTON(ID_ADDMENUITEM, MenuEditor::OnAddMenuItem)
EVT_BUTTON(ID_ADDSEPARATOR, MenuEditor::OnAddSeparator)
EVT_BUTTON(ID_MODIFYMENUITEM, MenuEditor::OnModifyMenuItem)
EVT_BUTTON(ID_REMOVEMENUITEM, MenuEditor::OnRemoveMenuItem)
EVT_BUTTON(ID_MENUDOWN, MenuEditor::OnMenuDown)
EVT_BUTTON(ID_MENULEFT, MenuEditor::OnMenuLeft)
EVT_BUTTON(ID_MENURIGHT, MenuEditor::OnMenuRight)
EVT_BUTTON(ID_MENUUP, MenuEditor::OnMenuUp)
EVT_UPDATE_UI_RANGE(ID_MENUDOWN, ID_MENUUP, MenuEditor::OnUpdateMovers)
EVT_TEXT_ENTER(wxID_ANY, MenuEditor::OnEnter)
EVT_TEXT(ID_LABEL, MenuEditor::OnLabelChanged)
EVT_LIST_ITEM_ACTIVATED(wxID_ANY, MenuEditor::OnItemActivated)
END_EVENT_TABLE()
#endif

MenuEditor::MenuEditor(wxWindow* parent, int id)
    : wxDialog(parent, id, _("Menu Editor"), wxDefaultPosition, wxDefaultSize)
    , m_menuList(new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL | wxSTATIC_BORDER))
    , m_tcId(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                            wxTE_PROCESS_ENTER))
    , m_tcLabel(new wxTextCtrl(this, ID_LABEL, "", wxDefaultPosition,
                               wxDefaultSize, wxTE_PROCESS_ENTER))
    , m_tcName(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                              wxDefaultSize, wxTE_PROCESS_ENTER))
    , m_tcHelpString(new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                    wxDefaultSize, wxTE_PROCESS_ENTER))
    , m_tcShortcut(new wxTextCtrl(this, ID_LABEL, "", wxDefaultPosition,
                                  wxDefaultSize, wxTE_PROCESS_ENTER))
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* sizerTop = new wxBoxSizer(wxHORIZONTAL);

    m_menuList->InsertColumn(0, _("Label"), wxLIST_FORMAT_LEFT, 150);
    m_menuList->InsertColumn(1, _("Shortcut"), wxLIST_FORMAT_LEFT, 80);
    m_menuList->InsertColumn(2, _("Id"), wxLIST_FORMAT_LEFT, 80);
    m_menuList->InsertColumn(3, _("Name"), wxLIST_FORMAT_LEFT, 50);
    m_menuList->InsertColumn(4, _("Help String"), wxLIST_FORMAT_LEFT, 150);
    m_menuList->InsertColumn(5, _("Kind"), wxLIST_FORMAT_LEFT, 120);

    int width = 0;
    for (int i = 0; i < m_menuList->GetColumnCount(); ++i)
        width += m_menuList->GetColumnWidth(i);

    m_menuList->SetMinSize(wxSize(width, -1));

    sizerTop->Add(m_menuList, 1, wxALL | wxEXPAND, 5);

    wxStaticBoxSizer* sizer1 = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _("Menu item")), wxVERTICAL);

    const auto sizer11 = new wxFlexGridSizer(2, 0, 0);
    sizer11->AddGrowableCol(1);

    wxStaticText* stLabel = new wxStaticText(
        this, wxID_ANY, _("Label"), wxDefaultPosition, wxDefaultSize, 0);

    sizer11->Add(stLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    sizer11->Add(m_tcLabel, 0, wxALL | wxEXPAND, 5);

    wxStaticText* stShortcut = new wxStaticText(
        this, wxID_ANY, _("Shortcut"), wxDefaultPosition, wxDefaultSize, 0);

    sizer11->Add(stShortcut, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    sizer11->Add(m_tcShortcut, 0, wxALL | wxEXPAND, 5);

    wxStaticText* stId = new wxStaticText(
        this, wxID_ANY, _("Id"), wxDefaultPosition, wxDefaultSize, 0);

    sizer11->Add(stId, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    sizer11->Add(m_tcId, 0, wxALL | wxEXPAND, 5);

    wxStaticText* stName = new wxStaticText(
        this, wxID_ANY, _("Name"), wxDefaultPosition, wxDefaultSize, 0);

    sizer11->Add(stName, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    sizer11->Add(m_tcName, 0, wxALL | wxEXPAND, 5);

    wxStaticText* stHelpString = new wxStaticText(
        this, wxID_ANY, _("Help String"), wxDefaultPosition, wxDefaultSize, 0);

    sizer11->Add(stHelpString, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    sizer11->Add(m_tcHelpString, 0, wxALL | wxEXPAND, 5);
    sizer1->Add(sizer11, 0, wxALL | wxEXPAND, 0);

    wxString choices[] = { "Normal", "Check", "Radio" };
    m_rbItemKind = new wxRadioBox(
        this, wxID_ANY, _("Kind"), wxDefaultPosition, wxDefaultSize,
        3, choices, 1, wxRA_SPECIFY_ROWS);

    sizer1->Add(m_rbItemKind, 0, wxALL | wxEXPAND, 5);

    wxBoxSizer* sizer4 = new wxBoxSizer(wxHORIZONTAL);

    wxButton* bAdd = new wxButton(
        this, ID_ADDMENUITEM, _("&Add"), wxDefaultPosition, wxDefaultSize, 0);

    sizer4->Add(bAdd, 1, wxALL, 5);

    wxButton* bModify = new wxButton(
        this, ID_MODIFYMENUITEM, _("&Modify"), wxDefaultPosition, wxDefaultSize, 0);

    sizer4->Add(bModify, 1, wxALL, 5);

    wxButton* bRemove = new wxButton(
        this, ID_REMOVEMENUITEM, _("&Remove"), wxDefaultPosition, wxDefaultSize, 0);

    sizer4->Add(bRemove, 1, wxALL, 5);
    sizer1->Add(sizer4, 0, wxEXPAND, 5);

    wxButton* bAddSep = new wxButton(
        this, ID_ADDSEPARATOR, _("Add &Separator"), wxDefaultPosition, wxDefaultSize, 0);

    sizer1->Add(bAddSep, 0, wxALL | wxEXPAND, 5);
    sizerTop->Add(sizer1, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(sizerTop, 1, wxEXPAND, 5);

    wxBoxSizer* sizerMoveButtons = new wxBoxSizer(wxHORIZONTAL);
    wxButton* bUp = new wxButton(
        this, ID_MENUUP, _("&Up"), wxDefaultPosition, wxDefaultSize, 0);

    sizerMoveButtons->Add(bUp, 0, wxALL, 5);

    wxButton* bDown = new wxButton(
        this, ID_MENUDOWN, _("&Down"), wxDefaultPosition, wxDefaultSize, 0);

    sizerMoveButtons->Add(bDown, 0, wxALL, 5);

    wxButton* bLeft = new wxButton(
        this, ID_MENULEFT, "<", wxDefaultPosition, wxDefaultSize, 0);

    sizerMoveButtons->Add(bLeft, 0, wxALL, 5);

    wxButton* bRight = new wxButton(
        this, ID_MENURIGHT, ">", wxDefaultPosition, wxDefaultSize, 0);

    sizerMoveButtons->Add(bRight, 0, wxALL, 5);

    wxStdDialogButtonSizer* sizerOkCancel = new wxStdDialogButtonSizer();
    sizerOkCancel->AddButton(new wxButton(this, wxID_OK));
    sizerOkCancel->AddButton(new wxButton(this, wxID_CANCEL));
    sizerOkCancel->Realize();
    sizerMoveButtons->Add(sizerOkCancel, 1, wxALL, 5);
    mainSizer->Add(sizerMoveButtons, 0, wxEXPAND | wxALIGN_CENTER_VERTICAL, 5);
    this->SetSizer(mainSizer);
    mainSizer->SetSizeHints(this);
    this->SetAutoLayout(true);
    this->Layout();
#if 0
    SetClientSize(560, 368);
#endif
    CenterOnScreen();

    Bind(wxEVT_BUTTON, &MenuEditor::OnAddMenuItem, this, ID_ADDMENUITEM);
    Bind(wxEVT_BUTTON, &MenuEditor::OnAddSeparator, this, ID_ADDSEPARATOR);
    Bind(wxEVT_BUTTON, &MenuEditor::OnModifyMenuItem, this, ID_MODIFYMENUITEM);
    Bind(wxEVT_BUTTON, &MenuEditor::OnRemoveMenuItem, this, ID_REMOVEMENUITEM);
    Bind(wxEVT_BUTTON, &MenuEditor::OnMenuDown, this, ID_MENUDOWN);
    Bind(wxEVT_BUTTON, &MenuEditor::OnMenuLeft, this, ID_MENULEFT);
    Bind(wxEVT_BUTTON, &MenuEditor::OnMenuRight, this, ID_MENURIGHT);
    Bind(wxEVT_BUTTON, &MenuEditor::OnMenuUp, this, ID_MENUUP);
    Bind(wxEVT_UPDATE_UI, &MenuEditor::OnUpdateMovers, this, ID_MENUDOWN, ID_MENUUP);
    Bind(wxEVT_TEXT_ENTER, &MenuEditor::OnEnter, this);
    Bind(wxEVT_TEXT, &MenuEditor::OnLabelChanged, this, ID_LABEL);
    Bind(wxEVT_LIST_ITEM_ACTIVATED, &MenuEditor::OnItemActivated, this);
}

void MenuEditor::AddChild(long& n, int ident, PObjectBase obj)
{
    for (size_t i = 0; i < obj->GetChildCount(); i++) {
        PObjectBase childObj = obj->GetChild(i);
        if (childObj->GetClassName() == "wxMenuItem") {
            InsertItem(
                n++,
                wxString(wxChar(' '), ident * IDENTATION) + childObj->GetPropertyAsString("label"),

                childObj->GetPropertyAsString("shortcut"),
                childObj->GetPropertyAsString("id"),
                childObj->GetPropertyAsString("name"),
                childObj->GetPropertyAsString("help"),
                childObj->GetPropertyAsString("kind"),
                childObj);
        } else if (childObj->GetClassName() == "separator") {
            InsertItem(
                n++,
                wxString(wxChar(' '), ident * IDENTATION) + "---", "", "", "", "", "", childObj);
        } else {
            InsertItem(n++, wxString(wxChar(' '), ident * IDENTATION) + childObj->GetPropertyAsString("label"),
                       "",
                       childObj->GetPropertyAsString("id"),
                       childObj->GetPropertyAsString("name"),
                       childObj->GetPropertyAsString("help"),
                       "",
                       childObj);
            AddChild(n, ident + 1, childObj);
        }
    }
}

void MenuEditor::Populate(PObjectBase obj)
{
    assert(obj && obj->GetClassName() == "wxMenuBar");
    long n = 0;
    AddChild(n, 0, obj);
}

bool MenuEditor::HasChildren(long n)
{
    if (n == m_menuList->GetItemCount() - 1)
        return false;
    else
        return GetItemIdentation(n + 1) > GetItemIdentation(n);
}

PObjectBase MenuEditor::GetMenu(long& n, PObjectDatabase base, bool isSubMenu)
{
    // Get item from list control
    wxString label, shortcut, id, name, help, kind;
    PObjectBase menu;
    GetItem(n, label, shortcut, id, name, help, kind, &menu);

    bool createNew = true;
    if (menu)
        createNew = (menu->GetClassName() != (isSubMenu ? "submenu" : "wxMenu"));

    // preserve original menu if the object types match
    // this preserves properties that are not exposed in the menu editor - like C++ scope
    if (createNew) {
        PObjectInfo info = base->GetObjectInfo(isSubMenu ? "submenu" : "wxMenu");
        menu = base->NewObject(info);
    }
    label.Trim(true);
    label.Trim(false);
    menu->GetProperty("label")->SetValue(label);
    menu->GetProperty("name")->SetValue(name);

    int ident = GetItemIdentation(n);
    n++;
    while (n < m_menuList->GetItemCount() && GetItemIdentation(n) > ident) {
        PObjectBase menuitem;
        GetItem(n, label, shortcut, id, name, help, kind, &menuitem);

        createNew = true;

        label.Trim(true);
        label.Trim(false);
        if (label == "---") {
            if (menuitem)
                createNew = (menuitem->GetClassName() != "separator");

            if (createNew) {
                PObjectInfo info = base->GetObjectInfo("separator");
                menuitem = base->NewObject(info);
            }
            menu->AddChild(menuitem);
            menuitem->SetParent(menu);
            n++;
        } else if (HasChildren(n)) {
            PObjectBase child = GetMenu(n, base);
            menu->AddChild(child);
            child->SetParent(menu);
        } else {
            if (menuitem)
                createNew = (menuitem->GetClassName() != "wxMenuItem");

            if (createNew) {
                PObjectInfo info = base->GetObjectInfo("wxMenuItem");
                menuitem = base->NewObject(info);
            }
            menuitem->GetProperty("label")->SetValue(label);
            menuitem->GetProperty("shortcut")->SetValue(shortcut);
            menuitem->GetProperty("name")->SetValue(name);
            menuitem->GetProperty("help")->SetValue(help);
            menuitem->GetProperty("id")->SetValue(id);
            menuitem->GetProperty("kind")->SetValue(kind);
            menu->AddChild(menuitem);
            menuitem->SetParent(menu);
            n++;
        }
    }
    return menu;
}

PObjectBase MenuEditor::GetMenubar(PObjectDatabase base)
{
    // Disconnect all parent/child relationships in original objects
    for (std::vector<WPObjectBase>::iterator it = m_originalItems.begin();
         it != m_originalItems.end(); ++it) {
        PObjectBase obj = it->lock();
        if (obj) {
            obj->RemoveAllChildren();
            obj->SetParent(PObjectBase());
        }
    }
    PObjectInfo info = base->GetObjectInfo("wxMenuBar");
    PObjectBase menubar = base->NewObject(info);
    long n = 0;
    while (n < m_menuList->GetItemCount()) {
        PObjectBase child = GetMenu(n, base, false);
        menubar->AddChild(child);
        child->SetParent(menubar);
    }
    return menubar;
}

long MenuEditor::GetSelectedItem()
{
    return m_menuList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

int MenuEditor::GetItemIdentation(long n)
{
    wxString label = m_menuList->GetItemText(n);
    size_t curIdent = 0;
    while (curIdent < label.Len() && label[curIdent] == wxChar(' '))
        curIdent++;
    curIdent /= IDENTATION;
    return (int)curIdent;
}

long MenuEditor::InsertItem(long n, const wxString& label, const wxString& shortcut,
                            const wxString& id, const wxString& name,
                            const wxString& helpString, const wxString& kind,
                            PObjectBase obj)
{
    long index = m_menuList->InsertItem(n, label);
    m_menuList->SetItem(index, 1, shortcut);
    m_menuList->SetItem(index, 2, id);
    m_menuList->SetItem(index, 3, name);
    m_menuList->SetItem(index, 4, helpString);
    m_menuList->SetItem(index, 5, kind);

    // Prevent loss of data not exposed by menu editor. For Example: bitmaps, events, etc.
    if (obj) {
        m_originalItems.push_back(obj);
        m_menuList->SetItemData(index, m_originalItems.size() - 1);
    } else {
        m_menuList->SetItemData(index, -1);
    }
    return index;
}

void MenuEditor::AddItem(const wxString& label, const wxString& shortcut,
                         const wxString& id, const wxString& name,
                         const wxString& help, const wxString& kind)
{
    int sel = GetSelectedItem();
    int identation = 0;
    if (sel >= 0)
        identation = GetItemIdentation(sel);
    wxString labelAux = label;
    labelAux.Trim(true);
    labelAux.Trim(false);
    if (sel < 0)
        sel = m_menuList->GetItemCount() - 1;

    labelAux = wxString(wxChar(' '), identation * IDENTATION) + labelAux;

    long index = InsertItem(sel + 1, labelAux, shortcut, id, name, help, kind);
    m_menuList->SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

void MenuEditor::GetItem(long n, wxString& label, wxString& shortcut,
                         wxString& id, wxString& name, wxString& help,
                         wxString& kind, PObjectBase* obj)
{
    label = m_menuList->GetItemText(n);
    wxListItem item;
    item.m_itemId = n;
    item.m_col = 1;
    item.m_mask = wxLIST_MASK_TEXT;
    m_menuList->GetItem(item);
    shortcut = item.GetText();
    item.m_col++;
    m_menuList->GetItem(item);
    id = item.GetText();
    if (id.empty())
        id = "wxID_ANY";

    item.m_col++;
    m_menuList->GetItem(item);
    name = item.GetText();
    item.m_col++;
    m_menuList->GetItem(item);
    help = item.GetText();
    item.m_col++;
    m_menuList->GetItem(item);
    kind = item.GetText();
    if (kind.empty()) {
        kind = "wxITEM_NORMAL";
    }
    if (obj) {
        int origIndex = m_menuList->GetItemData(item);
        if (origIndex >= 0 && origIndex < (int)m_originalItems.size())
            *obj = m_originalItems[origIndex].lock();
    }
}

void MenuEditor::AddNewItem()
{
    wxString kind;
    switch (m_rbItemKind->GetSelection()) {
    case 0:
        kind = "wxITEM_NORMAL";
        break;
    case 1:
        kind = "wxITEM_CHECK";
        break;
    case 2:
        kind = "wxITEM_RADIO";
        break;
    }
    AddItem(m_tcLabel->GetValue(), m_tcShortcut->GetValue(), m_tcId->GetValue(),
            m_tcName->GetValue(), m_tcHelpString->GetValue(), kind);
    m_tcLabel->SetValue("");
    m_tcShortcut->SetValue("");
    m_tcId->SetValue("");
    m_tcName->SetValue("");
    m_tcHelpString->SetValue("");
    m_rbItemKind->SetSelection(0);
    m_tcLabel->SetFocus();
}

void MenuEditor::OnAddMenuItem(wxCommandEvent&)
{
    AddNewItem();
}

void MenuEditor::OnAddSeparator(wxCommandEvent&)
{
    AddItem("---", "", "", "", "", "");
}

void MenuEditor::OnModifyMenuItem(wxCommandEvent&)
{
    long index = GetSelectedItem();
    int identation = GetItemIdentation(index);
    wxString kind;
    switch (m_rbItemKind->GetSelection()) {
    case 0:
        kind = "wxITEM_NORMAL";
        break;
    case 1:
        kind = "wxITEM_CHECK";
        break;
    case 2:
        kind = "wxITEM_RADIO";
        break;
    }
    m_menuList->SetItem(index, 0, wxString(wxChar(' '), identation * IDENTATION) + m_tcLabel->GetValue());
    m_menuList->SetItem(index, 1, m_tcShortcut->GetValue());
    m_menuList->SetItem(index, 2, m_tcId->GetValue());
    m_menuList->SetItem(index, 3, m_tcName->GetValue());
    m_menuList->SetItem(index, 4, m_tcHelpString->GetValue());
    m_menuList->SetItem(index, 5, kind);
}

void MenuEditor::OnRemoveMenuItem(wxCommandEvent&)
{
    long sel = GetSelectedItem();
    if (sel < m_menuList->GetItemCount() - 1) {
        int curIdent = GetItemIdentation(sel);
        int nextIdent = GetItemIdentation(sel + 1);
        if (nextIdent > curIdent) {
            int res = wxMessageBox(
                _("The children of the selected item will be eliminated too. Are you sure you want to continue?"),
                wxTheApp->GetAppDisplayName(), wxYES_NO);
            if (res == wxYES) {
                long item = sel + 1;
                while (item < m_menuList->GetItemCount() && GetItemIdentation(item) > curIdent)
                    m_menuList->DeleteItem(item);
                m_menuList->DeleteItem(sel);
            }
        } else
            m_menuList->DeleteItem(sel);
    } else
        m_menuList->DeleteItem(sel);
}

void MenuEditor::OnMenuLeft(wxCommandEvent&)
{
    int sel = GetSelectedItem();
    int curIdent = GetItemIdentation(sel) - 1;
    int childIdent = sel < m_menuList->GetItemCount() - 1 ? GetItemIdentation(sel + 1) : -1;

    if (curIdent < 0 || (childIdent != -1 && abs(curIdent - childIdent) > 1))
        return;

    wxString label = m_menuList->GetItemText(sel);
    label.Trim(true);
    label.Trim(false);
    label = wxString(wxChar(' '), curIdent * IDENTATION) + label;
    m_menuList->SetItemText(sel, label);
}

void MenuEditor::OnMenuRight(wxCommandEvent&)
{
    int sel = GetSelectedItem();
    int curIdent = GetItemIdentation(sel) + 1;
    int parentIdent = sel > 0 ? GetItemIdentation(sel - 1) : -1;

    if (parentIdent == -1 || abs(curIdent - parentIdent) > 1)
        return;

    wxString label = m_menuList->GetItemText(sel);
    label.Trim(true);
    label.Trim(false);
    label = wxString(wxChar(' '), curIdent * IDENTATION) + label;
    m_menuList->SetItemText(sel, label);
}

void MenuEditor::OnMenuUp(wxCommandEvent&)
{
    long sel = GetSelectedItem();
    long prev = sel - 1;
    int prevIdent = GetItemIdentation(prev);
    int curIdent = GetItemIdentation(sel);
    if (prevIdent < curIdent)
        return;
    while (prevIdent > curIdent)
        prevIdent = GetItemIdentation(--prev);

    PObjectBase obj;
    wxString label, shortcut, id, name, help, kind;
    GetItem(sel, label, shortcut, id, name, help, kind, &obj);

    m_menuList->DeleteItem(sel);
    long newSel = InsertItem(prev, label, shortcut, id, name, help, kind, obj);
    sel++;
    prev++;
    if (sel < m_menuList->GetItemCount()) {
        long childIdent = GetItemIdentation(sel);
        while (sel < m_menuList->GetItemCount() && childIdent > curIdent) {
            PObjectBase childObj;
            GetItem(sel, label, shortcut, id, name, help, kind, &childObj);
            m_menuList->DeleteItem(sel);
            InsertItem(prev, label, shortcut, id, name, help, kind, childObj);
            sel++;
            prev++;
            if (sel < m_menuList->GetItemCount())
                childIdent = GetItemIdentation(sel);
        }
    }
    m_menuList->SetItemState(newSel, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

long MenuEditor::GetEndIndex(long n)
{
    long res = n;
    int ident = GetItemIdentation(n);
    res++;
    if (res >= m_menuList->GetItemCount())
        return n;
    while (GetItemIdentation(res) > ident)
        res++;
    return res - 1;
}

void MenuEditor::OnMenuDown(wxCommandEvent&)
{
    long sel = GetSelectedItem();
    int selIdent = GetItemIdentation(sel);
    long selAux = sel + 1;
    while (GetItemIdentation(selAux) > selIdent)
        selAux++;
    if (GetItemIdentation(selAux) < selIdent)
        return;
    long endIndex = GetEndIndex(selAux) + 1;

    wxString label, shortcut, id, name, help, kind;
    PObjectBase obj;
    GetItem(sel, label, shortcut, id, name, help, kind, &obj);

    m_menuList->DeleteItem(sel);
    endIndex--;
    long first = InsertItem(endIndex, label, shortcut, id, name, help, kind, obj);
    while (GetItemIdentation(sel) > selIdent) {
        PObjectBase childObj;
        GetItem(sel, label, shortcut, id, name, help, kind, &childObj);
        m_menuList->DeleteItem(sel);
        InsertItem(endIndex, label, shortcut, id, name, help, kind, childObj);
        first--;
    }
    m_menuList->SetItemState(first, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
}

void MenuEditor::OnEnter(wxCommandEvent&)
{
    AddNewItem();
}

void MenuEditor::OnLabelChanged(wxCommandEvent&)
{
    wxString label = m_tcLabel->GetValue();
    wxString id, name;
    bool nextUpper = false;
    if (!label.IsEmpty())
        id = "ID_";

    int tabPos = label.Find("\\t");
    if (tabPos >= 0)
        label = label.Left(tabPos);

    for (size_t i = 0; i < label.Len(); i++) {
        if (isalnum(label[i])) {
            name += (nextUpper ? toupper(label[i]) : tolower(label[i]));
            nextUpper = false;
            id += toupper(label[i]);
        } else if (label[i] == wxChar(' ')) {
            nextUpper = true;
            id += "_";
        }
    }
    if (name.Len() > 0 && isdigit(name[0]))
        name = "n" + name;

    m_tcId->SetValue(id);
    m_tcName->SetValue(name);
}

void MenuEditor::OnUpdateMovers(wxUpdateUIEvent& e)
{
    switch (e.GetId()) {
    case ID_MENUUP:
        e.Enable(GetSelectedItem() > 0);
        break;
    case ID_MENUDOWN:
        e.Enable(GetSelectedItem() < m_menuList->GetItemCount() - 1);
        break;
    default:
        break;
    }
}

void MenuEditor::OnItemActivated(wxListEvent& e)
{
    wxString label, shortcut, id, name, helpString, kind;
    GetItem(e.GetIndex(), label, shortcut, id, name, helpString, kind);

    label.Trim(true);
    label.Trim(false);
    m_tcLabel->SetValue(label);
    m_tcShortcut->SetValue(shortcut);
    m_tcId->SetValue(id);
    m_tcName->SetValue(name);
    m_tcHelpString->SetValue(helpString);

    if (kind == "wxITEM_CHECK")
        m_rbItemKind->SetSelection(1);
    else if (kind == "wxITEM_RADIO")
        m_rbItemKind->SetSelection(2);
    else
        m_rbItemKind->SetSelection(0);
}
