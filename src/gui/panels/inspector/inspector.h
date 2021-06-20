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

#include "model/objectbase.h"

#include <wx/aui/auibook.h>
#include <wx/propgrid/manager.h>

#if !wxUSE_PROPGRID
#error "wxUSE_PROPGRID must be set to 1 in your wxWidgets library."
#endif

class wxWeaverEventHandlerEvent;
class wxWeaverPropertyEvent;
class wxWeaverObjectEvent;
class wxWeaverEvent;

enum {
    wxWEAVER_OI_DEFAULT_STYLE,
    wxWEAVER_OI_MULTIPAGE_STYLE,
    wxWEAVER_OI_SINGLE_PAGE_STYLE
};

class ObjectInspector : public wxPanel {
public:
    ObjectInspector(wxWindow* parent, int id, int style = wxWEAVER_OI_DEFAULT_STYLE);
    ~ObjectInspector() override;

    void OnObjectSelected(wxWeaverObjectEvent& event);
    void OnProjectRefresh(wxWeaverEvent& event);
    void OnPropertyModified(wxWeaverPropertyEvent& event);
    void OnEventHandlerModified(wxWeaverEventHandlerEvent& event);

    void AutoGenerateId(PObjectBase objectChanged, PProperty propChanged, wxString reason);
    wxPropertyGridManager* CreatePropertyGridManager(wxWindow* parent, wxWindowID id);
    void SaveSettings();

private:
    void Create(bool force = false);

    void AddItems(const wxString& name, PObjectBase obj, PObjectInfo objInfo,
                  PPropertyCategory category, PropertyMap& map);

    void AddItems(const wxString& name, PObjectBase obj, PObjectInfo objInfo,
                  PPropertyCategory category, EventMap& map);

    wxPGProperty* GetProperty(PProperty prop);

    void RestoreLastSelectedPropItem();
    void ModifyProperty(PProperty prop, const wxString& str);
    int StringToBits(const wxString& strVal, wxPGChoices& constants);

    void OnPropertyGridChanging(wxPropertyGridEvent& event);
    void OnPropertyGridChanged(wxPropertyGridEvent& event);
    void OnEventGridChanged(wxPropertyGridEvent& event);
    void OnPropertyGridDblClick(wxPropertyGridEvent& event);
    void OnEventGridDblClick(wxPropertyGridEvent& event);
    void OnPropertyGridExpand(wxPropertyGridEvent& event);
    void OnEventGridExpand(wxPropertyGridEvent& event);
    void OnPropertyGridItemSelected(wxPropertyGridEvent& event);
    void OnReCreateGrid(wxCommandEvent& event);
    void OnBitmapPropertyChanged(wxCommandEvent& event);
    void OnChildFocus(wxChildFocusEvent& event);

    template <class ValueT>
    void CreateCategory(const wxString& name, PObjectBase obj, PObjectInfo objInfo,
                        std::map<wxString, ValueT>& itemMap, bool addingEvents)
    {
        // Get Category
        PPropertyCategory category = objInfo->GetCategory();
        if (!category)
            return;

        // Prevent page creation if there are no properties
        if (!category->GetCategoryCount()
            && !(addingEvents
                     ? category->GetEventCount()
                     : category->GetPropertyCount()))
            return;

        wxString pageName;
        if (m_style == wxWEAVER_OI_MULTIPAGE_STYLE)
            pageName = name;
        else
            pageName = "default";

        wxPropertyGridManager* pg = (addingEvents ? m_eg : m_pg);
        int pageIndex = pg->GetPageByName(pageName);
        if (pageIndex == wxNOT_FOUND)
            pg->AddPage(pageName, objInfo->GetSmallIconFile());

        wxString catName = category->GetName();

        wxPGProperty* id = pg->Append(new wxPropertyCategory(_(catName), catName));
        id->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 0);

        AddItems(name, obj, objInfo, category, itemMap);

        ExpandMap::iterator it = m_isExpanded.find(catName);
        if (it != m_isExpanded.end()) {
            if (it->second)
                pg->Expand(id);
            else
                pg->Collapse(id);
        }
        pg->SetPropertyAttributeAll(wxPG_BOOL_USE_CHECKBOX, (long)1);
    }
    typedef std::map<wxPGProperty*, PProperty> ObjInspectorPropertyMap;
    typedef std::map<wxPGProperty*, PEvent> ObjInspectorEventMap;
    ObjInspectorPropertyMap m_propMap;
    ObjInspectorEventMap m_eventMap;

    PObjectBase m_currentSel;

    //save the current selected property
    wxString m_strSelPropItem;
    wxString m_pageName;

    wxAuiNotebook* m_nb;
    wxPropertyGridManager* m_pg;
    wxPropertyGridManager* m_eg;

    int m_style;

    typedef std::map<wxString, bool> ExpandMap;
    ExpandMap m_isExpanded;
};
