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
#include "gui/panels/inspector/inspector.h"
#include "gui/panels/inspector/advprops.h"

#include "utils/debug.h"
#include "utils/typeconv.h"
#include "utils/exception.h"

#include "appdata.h"
#include "gui/aui/tabart.h"
#include "gui/bitmaps.h"
#include "event.h"

#include <wx/config.h>
#include <wx/propgrid/propgrid.h>

wxDEFINE_EVENT(wxEVT_WVR_PROP_BITMAP_CHANGED, wxCommandEvent);

enum {
    wxWEAVER_PROPERTY_GRID = wxID_HIGHEST + 1000,
    wxWEAVER_EVENT_GRID,
};

// -----------------------------------------------------------------------
// ObjectInspector
// -----------------------------------------------------------------------
#if 0
BEGIN_EVENT_TABLE(ObjectInspector, wxPanel)
EVT_PG_CHANGING(wxWEAVER_PROPERTY_GRID, ObjectInspector::OnPropertyGridChanging)
EVT_PG_CHANGED(wxWEAVER_PROPERTY_GRID, ObjectInspector::OnPropertyGridChanged)
EVT_PG_CHANGED(wxWEAVER_EVENT_GRID, ObjectInspector::OnEventGridChanged)
EVT_PG_DOUBLE_CLICK(wxWEAVER_EVENT_GRID, ObjectInspector::OnEventGridDblClick)
EVT_PG_DOUBLE_CLICK(wxWEAVER_PROPERTY_GRID, ObjectInspector::OnPropertyGridDblClick)
EVT_PG_ITEM_COLLAPSED(wxWEAVER_PROPERTY_GRID, ObjectInspector::OnPropertyGridExpand)
EVT_PG_ITEM_EXPANDED(wxWEAVER_PROPERTY_GRID, ObjectInspector::OnPropertyGridExpand)
EVT_PG_ITEM_COLLAPSED(wxWEAVER_EVENT_GRID, ObjectInspector::OnEventGridExpand)
EVT_PG_ITEM_EXPANDED(wxWEAVER_EVENT_GRID, ObjectInspector::OnEventGridExpand)
EVT_PG_SELECTED(wxWEAVER_PROPERTY_GRID, ObjectInspector::OnPropertyGridItemSelected)
EVT_PG_SELECTED(wxWEAVER_EVENT_GRID, ObjectInspector::OnPropertyGridItemSelected)

EVT_WVR_OBJECT_SELECTED(ObjectInspector::OnObjectSelected)
EVT_WVR_PROJECT_REFRESH(ObjectInspector::OnProjectRefresh)
EVT_WVR_PROPERTY_MODIFIED(ObjectInspector::OnPropertyModified)
EVT_WVR_EVENT_HANDLER_MODIFIED(ObjectInspector::OnEventHandlerModified)

EVT_CHILD_FOCUS(ObjectInspector::OnChildFocus)
END_EVENT_TABLE()
#endif

ObjectInspector::ObjectInspector(wxWindow* parent, int id, int style)
    : wxPanel(parent, id)
    , m_currentSel(PObjectBase())
    , m_nb(new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP))
    , m_style(style)
{
    AppData()->AddHandler(this->GetEventHandler());

    m_nb->SetArtProvider(new AuiTabArt());

    // TODO: This seems to make no difference in wxGTK 3.0.5.1
    // The colour of property grid description looks ugly if we don't set this colour
    m_nb->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
#if 0
// This is here for historical reason, when using a slider as editor to set
// values for the wxFloatProperty, but never implemented in wx 2.9+.
#if wxUSE_SLIDER
    wxPGRegisterEditorClass(Slider); // Register the slider editor
#endif
#endif
    m_pg = CreatePropertyGridManager(m_nb, wxWEAVER_PROPERTY_GRID);
    m_eg = CreatePropertyGridManager(m_nb, wxWEAVER_EVENT_GRID);

    m_nb->AddPage(m_pg, _("Properties"), false, 0);
    m_nb->AddPage(m_eg, _("Events"), false, 1);

    m_nb->SetPageBitmap(0, AppBitmaps::GetBitmap("properties", 16));
    m_nb->SetPageBitmap(1, AppBitmaps::GetBitmap("events", 16));

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_nb, 1, wxALL | wxEXPAND, 0);
    SetSizer(topSizer);

    Bind(wxEVT_CHILD_FOCUS, &ObjectInspector::OnChildFocus, this);

    Bind(wxEVT_PG_CHANGING, &ObjectInspector::OnPropertyGridChanging, this, wxWEAVER_PROPERTY_GRID);
    Bind(wxEVT_PG_CHANGED, &ObjectInspector::OnPropertyGridChanged, this, wxWEAVER_PROPERTY_GRID);
    Bind(wxEVT_PG_CHANGED, &ObjectInspector::OnEventGridChanged, this, wxWEAVER_EVENT_GRID);
    Bind(wxEVT_PG_DOUBLE_CLICK, &ObjectInspector::OnEventGridDblClick, this, wxWEAVER_EVENT_GRID);
    Bind(wxEVT_PG_DOUBLE_CLICK, &ObjectInspector::OnPropertyGridDblClick, this, wxWEAVER_PROPERTY_GRID);
    Bind(wxEVT_PG_ITEM_COLLAPSED, &ObjectInspector::OnPropertyGridExpand, this, wxWEAVER_PROPERTY_GRID);
    Bind(wxEVT_PG_ITEM_EXPANDED, &ObjectInspector::OnPropertyGridExpand, this, wxWEAVER_PROPERTY_GRID);
    Bind(wxEVT_PG_ITEM_COLLAPSED, &ObjectInspector::OnEventGridExpand, this, wxWEAVER_EVENT_GRID);
    Bind(wxEVT_PG_ITEM_EXPANDED, &ObjectInspector::OnEventGridExpand, this, wxWEAVER_EVENT_GRID);
    Bind(wxEVT_PG_SELECTED, &ObjectInspector::OnPropertyGridItemSelected, this, wxWEAVER_PROPERTY_GRID);
    Bind(wxEVT_PG_SELECTED, &ObjectInspector::OnPropertyGridItemSelected, this, wxWEAVER_EVENT_GRID);

    Bind(wxEVT_WVR_OBJECT_SELECTED, &ObjectInspector::OnObjectSelected, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &ObjectInspector::OnProjectRefresh, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &ObjectInspector::OnPropertyModified, this);
    Bind(wxEVT_WVR_EVENT_HANDLER_MODIFIED, &ObjectInspector::OnEventHandlerModified, this);

    Bind(wxEVT_WVR_PROP_BITMAP_CHANGED, &ObjectInspector::OnBitmapPropertyChanged, this);
}

ObjectInspector::~ObjectInspector()
{
    Unbind(wxEVT_WVR_PROP_BITMAP_CHANGED, &ObjectInspector::OnBitmapPropertyChanged, this);

    AppData()->RemoveHandler(this->GetEventHandler());
}

void ObjectInspector::SaveSettings()
{
    // Save Layout
    wxConfigBase* config = wxConfigBase::Get();
    config->Write("/PropertyEditor/DescBoxHeight", m_pg->GetDescBoxHeight());
}

void ObjectInspector::Create(bool force)
{
    PObjectBase selectedObject = AppData()->GetSelectedObject();
    if (selectedObject && (selectedObject != m_currentSel || force)) {

        Freeze();

        m_currentSel = selectedObject;

        int pageNumber = m_pg->GetSelectedPage();
        wxString pageName;
        if (pageNumber != wxNOT_FOUND)
            pageName = m_pg->GetPageName(pageNumber);

        m_pg->Clear(); // Clear PropertyGridManager
        m_eg->Clear(); // Now we do the same thing for event grid...

        m_propMap.clear();
        m_eventMap.clear();

        PObjectInfo objDescription = selectedObject->GetObjectInfo();
        if (objDescription) {
            PropertyMap propMap, dummyPropMap;
            EventMap eventMap, dummyEventMap;

            // We create the categories with the properties of the object organized by "classes"
            CreateCategory(objDescription->GetClassName(), selectedObject,
                           objDescription, propMap, false);

            CreateCategory(objDescription->GetClassName(), selectedObject,
                           objDescription, eventMap, true);

            for (size_t i = 0; i < objDescription->GetBaseClassCount(); i++) {
                PObjectInfo infoBase = objDescription->GetBaseClass(i);
                CreateCategory(infoBase->GetClassName(), selectedObject,
                               infoBase, propMap, false);

                CreateCategory(infoBase->GetClassName(), selectedObject,
                               infoBase, eventMap, true);
            }
            PObjectBase parent = selectedObject->GetParent();
            if (parent) {
                PObjectInfo parentDescription = parent->GetObjectInfo();
                if (parentDescription->GetType()->IsItem()) {
                    CreateCategory(parentDescription->GetClassName(), parent,
                                   parentDescription, dummyPropMap, false);

                    CreateCategory(parentDescription->GetClassName(), parent,
                                   parentDescription, dummyEventMap, true);

                    for (size_t i = 0; i < parentDescription->GetBaseClassCount(); i++) {
                        PObjectInfo infoBase = parentDescription->GetBaseClass(i);
                        CreateCategory(infoBase->GetClassName(), parent,
                                       infoBase, dummyPropMap, false);

                        CreateCategory(infoBase->GetClassName(), parent,
                                       infoBase, dummyEventMap, true);
                    }
                }
            }
            // Select previously selected page, or first page
            if (m_pg->GetPageCount() > 0) {
                int pageIndex = m_pg->GetPageByName(pageName);
                if (wxNOT_FOUND != pageIndex)
                    m_pg->SelectPage(pageIndex);
                else
                    m_pg->SelectPage(0);
            }
        }
        m_pg->Refresh();
        m_pg->Update();
        m_eg->Refresh();
        m_eg->Update();

        Thaw();

        RestoreLastSelectedPropItem();
    }
}

int ObjectInspector::StringToBits(const wxString& strVal, wxPGChoices& constants)
{
    wxStringTokenizer strTok(strVal, " |");
    int val = 0;
    while (strTok.HasMoreTokens()) {
        wxString token = strTok.GetNextToken();
        size_t i = 0;
        bool done = false;
        while (i < constants.GetCount() && !done) {
            if (constants.GetLabel(i) == token) {
                val |= constants.GetValue(i);
                done = true;
            }
            i++;
        }
    }
    return val;
}

wxPGProperty* ObjectInspector::GetProperty(PProperty prop)
{
    wxPGProperty* result = nullptr;
    PropertyType type = prop->GetType();
    wxString name = prop->GetName();
    wxVariant vTrue = wxVariant(true, "true");

    if (type == PT_MACRO) {
        result = new wxStringProperty(name, wxPG_LABEL,
                                      prop->GetValueAsString());
    } else if (type == PT_INT) {
        result = new wxIntProperty(name, wxPG_LABEL,
                                   prop->GetValueAsInteger());
    } else if (type == PT_UINT) {
        result = new wxUIntProperty(name, wxPG_LABEL,
                                    (unsigned)prop->GetValueAsInteger());
    } else if (type == PT_WXSTRING || type == PT_WXSTRING_I18N) {
        result = new wxLongStringProperty(name, wxPG_LABEL,
                                          prop->GetValueAsText());
    } else if (type == PT_TEXT) {
        result = new wxLongStringProperty(name, wxPG_LABEL,
                                          prop->GetValueAsString());
    } else if (type == PT_BOOL) {
        result = new wxBoolProperty(name, wxPG_LABEL,
                                    prop->GetValue() == "1");
    } else if (type == PT_BITLIST) {
        PPropertyInfo propDesc = prop->GetPropertyInfo();
        POptionList optList = propDesc->GetOptionList();

        assert(optList && optList->GetOptionCount() > 0);

        wxPGChoices constants;
        const std::map<wxString, wxString> options = optList->GetOptions();
        std::map<wxString, wxString>::const_iterator it;
        size_t index = 0;
        for (it = options.begin(); it != options.end(); ++it)
            constants.Add(it->first, 1 << index++);

        int val = StringToBits(prop->GetValueAsString(), constants);
        result = new wxFlagsProperty(name, wxPG_LABEL, constants, val);

        // Workaround to set the help strings for individual members of a wxFlagsProperty
        wxFlagsProperty* flagsProp = dynamic_cast<wxFlagsProperty*>(result);
        if (flagsProp) {
            for (size_t i = 0; i < flagsProp->GetItemCount(); i++) {
                wxPGProperty* itemProp = flagsProp->Item(i);
                std::map<wxString, wxString>::const_iterator option
                    = options.find(itemProp->GetLabel());
                if (option != options.end())
                    m_pg->SetPropertyHelpString(itemProp, option->second);
            }
        }
    } else if (type == PT_INTLIST
               || type == PT_UINTLIST
               || type == PT_INTPAIRLIST
               || type == PT_UINTPAIRLIST) {
        result = new wxStringProperty(
            name, wxPG_LABEL,
            IntList(prop->GetValueAsString(),
                    type == PT_UINTLIST, (PT_INTPAIRLIST == type || PT_UINTPAIRLIST == type))
                .ToString(true));
    } else if (type == PT_OPTION || type == PT_EDIT_OPTION) {
        PPropertyInfo propDesc = prop->GetPropertyInfo();
        POptionList optList = propDesc->GetOptionList();

        assert(optList && optList->GetOptionCount() > 0);

        wxString value = prop->GetValueAsString();
        wxString help;
        wxPGChoices constants;
        const std::map<wxString, wxString> options = optList->GetOptions();
        std::map<wxString, wxString>::const_iterator it;
        size_t i = 0;

        for (it = options.begin(); it != options.end(); ++it) {
            constants.Add(it->first, i++);
            if (it->first == value)
                help = it->second; // Save help
        }
        if (type == PT_EDIT_OPTION)
            result = new wxEditEnumProperty(name, wxPG_LABEL, constants);
        else
            result = new wxEnumProperty(name, wxPG_LABEL, constants);
        result->SetValueFromString(value, 0);

        wxString desc = propDesc->GetDescription();
        if (desc.empty())
            desc = value + ":\n" + help;
        else
            desc += "\n\n" + value + ":\n" + help;

        result->SetHelpString(wxGetTranslation(desc));

    } else if (type == PT_WXPOINT) {
        result = new wxWeaverPointProperty(name, wxPG_LABEL,
                                           prop->GetValueAsPoint());
    } else if (type == PT_WXSIZE) {
        result = new wxWeaverSizeProperty(name, wxPG_LABEL,
                                          prop->GetValueAsSize());
    } else if (type == PT_WXFONT) {
        result = new wxWeaverFontProperty(name, wxPG_LABEL,
                                          TypeConv::StringToFont(prop->GetValueAsString()));
    } else if (type == PT_WXCOLOUR) {
        wxString value = prop->GetValueAsString();
        if (value.empty()) // Default Colour
        {
            wxColourPropertyValue colProp;
            colProp.m_type = wxSYS_COLOUR_WINDOW;
            colProp.m_colour = TypeConv::StringToSystemColour("wxSYS_COLOUR_WINDOW");
            result = new wxSystemColourProperty(name, wxPG_LABEL, colProp);
        } else {
            if (!value.find_first_of("wx")) {
                wxColourPropertyValue def; // System Colour
                def.m_type = TypeConv::StringToSystemColour(value);
                result = new wxSystemColourProperty(name, wxPG_LABEL, def);
            } else {
                result = new wxSystemColourProperty(name, wxPG_LABEL,
                                                    prop->GetValueAsColour());
            }
        }
    } else if (type == PT_PATH) {
        result = new wxDirProperty(name, wxPG_LABEL, prop->GetValueAsString());
    } else if (type == PT_FILE) {
        result = new wxFileProperty(name, wxPG_LABEL, prop->GetValueAsString());
    } else if (type == PT_BITMAP) {
        wxLogDebug("OI::GetProperty: prop:%s", prop->GetValueAsString().c_str());

        result = new wxWeaverBitmapProperty(name, wxPG_LABEL, prop->GetValueAsString());
    } else if (type == PT_STRINGLIST) {
        result = new wxArrayStringProperty(name, wxPG_LABEL, prop->GetValueAsArrayString());
#if wxVERSION_NUMBER >= 2901
        wxVariant v("\"");
        result->DoSetAttribute(wxPG_ARRAY_DELIMITER, v);
#endif
    } else if (type == PT_FLOAT) {
        result = new wxFloatProperty(name, wxPG_LABEL, prop->GetValueAsFloat());
    } else if (type == PT_PARENT) {
        result = new wxStringProperty(name, wxPG_LABEL);
        result->ChangeFlag(wxPG_PROP_READONLY, true);
#if 0
#if 1
        wxPGProperty* parent = new wxPGProperty(name, wxPG_LABEL);
        parent->SetValueFromString(prop->GetValueAsString(), wxPG_FULL_VALUE);
#else
        wxPGProperty* parent = new wxStringProperty(name, wxPG_LABEL, "<composed>");
        parent->SetValueFromString(prop->GetValueAsString());
#endif
        PPropertyInfo propDesc = prop->GetPropertyInfo();
        std::list<PropertyChild>* children = propDesc->GetChildren();
        std::list<PropertyChild>::iterator it;
        for (it = children->begin(); it != children->end(); ++it) {
            wxPGProperty* child = new wxStringProperty(
                it->m_name, wxPG_LABEL, wxEmptyString);

            parent->AppendChild(child);
            m_pg->SetPropertyHelpString(child, it->m_description);
        }
        result = parent;
#endif
    } else // Unknown property
    {
        result = new wxStringProperty(name, wxPG_LABEL, prop->GetValueAsString());
        result->SetAttribute(wxPG_BOOL_USE_DOUBLE_CLICK_CYCLING, vTrue);
        wxLogError("Property type Unknown");
    }
    return result;
}

void ObjectInspector::AddItems(const wxString& name, PObjectBase obj,
                               PObjectInfo objInfo, PPropertyCategory category,
                               PropertyMap& properties)
{
    size_t propCount = category->GetPropertyCount();
    for (size_t i = 0; i < propCount; i++) {
        wxString propName = category->GetPropertyName(i);
        PProperty prop = obj->GetProperty(propName);

        if (!prop)
            continue;

        PPropertyInfo propInfo = prop->GetPropertyInfo();

        // we do not want to duplicate inherited properties
        if (properties.find(propName) == properties.end()) {
            wxPGProperty* id = m_pg->Append(GetProperty(prop));
            int propType = prop->GetType();
            if (propType != PT_OPTION) {

                m_pg->SetPropertyHelpString(id, propInfo->GetDescription());

                if (propType == PT_BITMAP) {
                    wxWeaverBitmapProperty* bp = wxDynamicCast(id, wxWeaverBitmapProperty);
                    if (bp) {
                        bp->CreateChildren();

                        // perform delayed child properties update
                        wxCommandEvent e(wxEVT_WVR_PROP_BITMAP_CHANGED);
                        e.SetString(bp->GetName() + ":" + prop->GetValue());
                        GetEventHandler()->AddPendingEvent(e);
#if 0
                        AppData()->ModifyProperty(prop, bp->GetValueAsString());
#endif
                    }
                } else if (propType == PT_PARENT) {
                    PPropertyInfo propDesc = prop->GetPropertyInfo();
                    std::list<PropertyChild>* children = propDesc->GetChildren();
                    std::list<PropertyChild>::iterator it;
                    wxArrayString values
                        = wxStringTokenize(prop->GetValueAsString(), ";", wxTOKEN_RET_EMPTY_ALL);
                    size_t index = 0;
                    wxString value;

                    for (it = children->begin(); it != children->end(); ++it) {
                        if (values.GetCount() > index)
                            value = values[index++].Trim().Trim(false);
                        else
                            value = "";

                        wxPGProperty* child = nullptr;
                        if (PT_BOOL == it->m_type) {
                            /*
    Because the format of a composed wxPGProperty value is stored this needs to
    be converted:

        true == "<property name>"
        false == "Not <property name>"

    TODO: The subclass property is currently the only one using this child type,
    because the only instance using this property, the c++ code generator,
    interprets a missing value as true and currently no project file update
    adds this value if it is missing, here a missing value also needs to be
    interpreted as true
*/
                            child = new wxBoolProperty(
                                it->m_name, wxPG_LABEL,
                                value.empty() || value == it->m_name);
                        } else if (PT_WXSTRING == it->m_type) {
                            child = new wxStringProperty(
                                it->m_name, wxPG_LABEL, value);
                        } else {
                            wxWEAVER_THROW_EX(
                                "Invalid Child Property Type: " << it->m_type);
                        }
                        id->AppendChild(child);
                        m_pg->SetPropertyHelpString(child, it->m_description);
                    }
                }
            }
            wxString customEditor = propInfo->GetCustomEditor();
            if (!customEditor.empty()) {
                wxPGEditor* editor = m_pg->GetEditorByName(customEditor);
                if (editor)
                    m_pg->SetPropertyEditor(id, editor);
            }
            if (m_style != wxWEAVER_OI_MULTIPAGE_STYLE) {
                // Most common classes will be showed with a slightly different colour.
                if (!AppData()->IsDarkMode()) {
                    if (name == "wxWindow")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(255, 250, 205)); // LemonChiffon
                    else if (name == "AUI")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(175, 238, 238)); // PaleTurquoise
                    else if (name == "sizeritem"
                             || name == "gbsizeritem"
                             || name == "sizeritembase")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(224, 255, 255)); // LightCyan
                } else {
                    if (name == "wxWindow")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(210, 105, 30)); // Chocolate
                    else if (name == "AUI")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(65, 105, 225)); // RoyalBlue
                    else if (name == "sizeritem"
                             || name == "gbsizeritem"
                             || name == "sizeritembase")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(0, 139, 139)); // DarkCyan
                }
            }
            ExpandMap::iterator it = m_isExpanded.find(propName);
            if (it != m_isExpanded.end()) {
                if (it->second)
                    m_pg->Expand(id);
                else
                    m_pg->Collapse(id);
            }
            properties.insert(PropertyMap::value_type(propName, prop));
            m_propMap.insert(ObjInspectorPropertyMap::value_type(id, prop));
        }
    }
    size_t catCount = category->GetCategoryCount();
    for (size_t i = 0; i < catCount; i++) {
        PPropertyCategory nextCat = category->GetCategory(i);
        if (!nextCat->GetCategoryCount() && !nextCat->GetPropertyCount())
            continue;

        wxPGProperty* catId
            = m_pg->AppendIn(category->GetName(),
                             new wxPropertyCategory(nextCat->GetName()));

        catId->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 0);

        AddItems(name, obj, objInfo, nextCat, properties);

        ExpandMap::iterator it = m_isExpanded.find(nextCat->GetName());
        if (it != m_isExpanded.end()) {
            if (it->second)
                m_pg->Expand(catId);
            else
                m_pg->Collapse(catId);
        }
    }
}

void ObjectInspector::AddItems(const wxString& name, PObjectBase obj,
                               PObjectInfo objInfo, PPropertyCategory category,
                               EventMap& events)
{
    size_t eventCount = category->GetEventCount();
    for (size_t i = 0; i < eventCount; i++) {
        wxString eventName = category->GetEventName(i);
        PEvent event = obj->GetEvent(eventName);

        if (!event)
            continue;

        PEventInfo eventInfo = event->GetEventInfo();

        // We do not want to duplicate inherited events
        if (events.find(eventName) == events.end()) {
            wxPGProperty* pgProp = new wxStringProperty(
                eventInfo->GetName(), wxPG_LABEL, event->GetValue());
            wxPGProperty* id = m_eg->Append(pgProp);

            m_eg->SetPropertyHelpString(id, eventInfo->GetDescription());

            if (m_style != wxWEAVER_OI_MULTIPAGE_STYLE) {
                if (!AppData()->IsDarkMode()) {
                    if (name == "wxWindow")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(255, 250, 205)); // LemonChiffon
                    else if (name == "AUI Events")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(175, 238, 238)); // PaleTurquoise
                } else {
                    if (name == "wxWindow")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(210, 105, 30)); // Chocolate
                    else if (name == "AUI Events")
                        m_pg->SetPropertyBackgroundColour(
                            id, wxColour(65, 105, 225)); // RoyalBlue
                }
            }
            ExpandMap::iterator it = m_isExpanded.find(eventName);
            if (it != m_isExpanded.end()) {
                if (it->second)
                    m_eg->Expand(id);
                else
                    m_eg->Collapse(id);
            }
            events.insert(EventMap::value_type(eventName, event));
            m_eventMap.insert(ObjInspectorEventMap::value_type(id, event));
        }
    }
    size_t catCount = category->GetCategoryCount();
    for (size_t i = 0; i < catCount; i++) {
        PPropertyCategory nextCat = category->GetCategory(i);
        if (!nextCat->GetCategoryCount() && !nextCat->GetEventCount())
            continue;

        wxPGProperty* catId = m_eg->AppendIn(
            category->GetName(), new wxPropertyCategory(nextCat->GetName()));

        catId->SetTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), 0);

        AddItems(name, obj, objInfo, nextCat, events);

        ExpandMap::iterator it = m_isExpanded.find(nextCat->GetName());
        if (it != m_isExpanded.end()) {
            if (it->second)
                m_eg->Expand(catId);
            else
                m_eg->Collapse(catId);
        }
    }
}

void ObjectInspector::OnPropertyGridChanging(wxPropertyGridEvent& event)
{
    wxImageFileProperty* imgFileProp
        = wxDynamicCast(event.GetProperty(), wxImageFileProperty);
    if (imgFileProp) {
        // GetValue() returns the pending value, but is only supported by wxEVT_PG_CHANGING.
        wxWeaverBitmapProperty* bmpProp = wxDynamicCast(imgFileProp->GetParent(), wxWeaverBitmapProperty);
        if (bmpProp) {
            wxString img = bmpProp->SetupImage(event.GetValue().GetString());
            if (img == wxEmptyString) {
                event.Veto();
                return;
            }
        }
    }
}

void ObjectInspector::OnPropertyGridChanged(wxPropertyGridEvent& event)
{
    wxPGProperty* propPtr = event.GetProperty();
    ObjInspectorPropertyMap::iterator it = m_propMap.find(propPtr);

    if (m_propMap.end() == it) {
        // Could be a child property
        propPtr = propPtr->GetParent();
        it = m_propMap.find(propPtr);
    }
    if (it != m_propMap.end()) {
        PProperty prop = it->second;
        PropertyType propType = prop->GetType();

        switch (propType) {
        case PT_FLOAT: {
            // Use typeconv to properly handle locale
            double val = m_pg->GetPropertyValueAsDouble(propPtr);
            ModifyProperty(prop, TypeConv::FloatToString(val));
            break;
        }
        case PT_TEXT:
        case PT_MACRO:
        case PT_INT:
        case PT_UINT: {
            ModifyProperty(prop, m_pg->GetPropertyValueAsString(propPtr));
            break;
        }
        case PT_OPTION:
        case PT_EDIT_OPTION: {
            wxString value = m_pg->GetPropertyValueAsString(propPtr);
            ModifyProperty(prop, value);

            // Update displayed description for the new selection
            PPropertyInfo propDesc = prop->GetPropertyInfo();
            POptionList optList = propDesc->GetOptionList();

            wxString helpString = propDesc->GetDescription();
            if (optList && optList->GetOptionCount() > 0) {
                const std::map<wxString, wxString> options = optList->GetOptions();
                std::map<wxString, wxString>::const_iterator option = options.find(value);
                if (option != options.end()) {
                    if (helpString.empty()) {
                        helpString = value + ":\n" + option->second;
                    } else {
                        helpString += "\n\n" + value + ":\n" + option->second;
                    }
                }
            }
            m_pg->SetPropertyHelpString(propPtr, helpString);
            m_pg->SetDescription(propPtr->GetLabel(), helpString);
            break;
        }
        case PT_PARENT: {
            // GenerateComposedValue() is the only method that does actually return a value,
            // although the documentation claims the other methods just call this one,
            // they return an empty value
            const auto value = propPtr->GenerateComposedValue();
            ModifyProperty(prop, value);
            break;
        }
        case PT_WXSTRING:
        case PT_WXSTRING_I18N: {
            // ObjectInspector's text strings are formatted.
            wxString value = TypeConv::TextToString(
                m_pg->GetPropertyValueAsString(propPtr));
            ModifyProperty(prop, value);
            break;
        }
        case PT_BOOL: {
            if (prop->GetName() == "aui_managed") {
                PObjectBase propobj = prop->GetObject();
                if (propobj->GetChildCount()) {
                    wxMessageBox(_("You have to remove all child widgets first."));
                    m_pg->SetPropertyValue(
                        propPtr, !m_pg->GetPropertyValueAsBool(propPtr));
                } else
                    ModifyProperty(
                        prop, m_pg->GetPropertyValueAsBool(propPtr) ? "1" : "0");
            } else
                ModifyProperty(
                    prop, m_pg->GetPropertyValueAsBool(propPtr) ? "1" : "0");
            break;
        }
        case PT_BITLIST: {
            wxString aux = m_pg->GetPropertyValueAsString(propPtr);
            aux.Replace(" ", "");
            aux.Replace(",", "|");
            ModifyProperty(prop, aux);
            break;
        }
        case PT_WXPOINT: {
            wxPoint point = wxPointRefFromVariant(event.GetPropertyValue());
            ModifyProperty(
                prop, wxString::Format("%i,%i", point.x, point.y));
            break;
        }
        case PT_WXSIZE: {
            wxSize size = wxSizeRefFromVariant(event.GetPropertyValue());
            ModifyProperty(
                prop, wxString::Format("%i,%i", size.GetWidth(), size.GetHeight()));
            break;
        }
        case PT_WXFONT: {
            ModifyProperty(prop, event.GetPropertyValue().GetString());
            break;
        }
        case PT_WXCOLOUR: {
            wxColourPropertyValue colour;
            colour << event.GetPropertyValue();
            switch (colour.m_type) {
            case wxSYS_COLOUR_MAX:
                ModifyProperty(prop, "");
                break;
            case wxPG_COLOUR_CUSTOM:
                ModifyProperty(prop, TypeConv::ColourToString(colour.m_colour));
                break;
            default:
                wxString sCol = TypeConv::SystemColourToString(colour.m_type);
                ModifyProperty(prop, sCol);
            }
            break;
        }
        case PT_INTLIST:
        case PT_UINTLIST:
        case PT_INTPAIRLIST:
        case PT_UINTPAIRLIST: {
            IntList intList(event.GetPropertyValue(),
                            propType == PT_UINTLIST,
                            (propType == PT_INTPAIRLIST
                             || propType == PT_UINTPAIRLIST));
            ModifyProperty(prop, intList.ToString(true));
            break;
        }
        case PT_BITMAP: {
            wxVariant childValue = event.GetProperty()->GetValue();

            // Also, handle the case where property value is unspecified
            if (childValue.IsNull())
                return;

            // bp->GetValue() have no updated value...
            wxString bmpVal = propPtr->GetValueAsString(wxPG_FULL_VALUE);

            // Handle changes in values, as needed
            wxVariant thisValue = WXVARIANT(bmpVal);
            wxVariant newVal = propPtr->ChildChanged(
                thisValue, (int)event.GetProperty()->GetIndexInParent(), childValue);

            ModifyProperty(prop, newVal.GetString());

            if (event.GetProperty()->GetIndexInParent() > 0) {
                // perform delayed child properties update
                wxCommandEvent e(wxEVT_WVR_PROP_BITMAP_CHANGED);
                e.SetString(propPtr->GetName() + ":" + bmpVal);
                GetEventHandler()->AddPendingEvent(e);
            }
            break;
        }
        default:
#if 0
            ModifyProperty(prop, event.GetPropertyValue());
#else
            ModifyProperty(prop, propPtr->GetValueAsString());
#endif
        }
    }
}

void ObjectInspector::OnEventGridChanged(wxPropertyGridEvent& event)
{
    ObjInspectorEventMap::iterator it = m_eventMap.find(event.GetProperty());
    if (it != m_eventMap.end()) {
        PEvent evt = it->second;
        wxString handler = event.GetPropertyValue();
        handler.Trim();
        handler.Trim(false);
        AppData()->ModifyEventHandler(evt, handler);
    }
}

void ObjectInspector::OnPropertyGridExpand(wxPropertyGridEvent& event)
{
    m_isExpanded[event.GetPropertyName()] = event.GetProperty()->IsExpanded();

    wxPGProperty* egProp = m_eg->GetProperty(event.GetProperty()->GetName());
    if (egProp) {
        if (event.GetProperty()->IsExpanded())
            m_eg->Expand(egProp);
        else
            m_eg->Collapse(egProp);
    }
}

void ObjectInspector::OnEventGridExpand(wxPropertyGridEvent& event)
{
    m_isExpanded[event.GetPropertyName()] = event.GetProperty()->IsExpanded();

    wxPGProperty* pgProp = m_pg->GetProperty(event.GetProperty()->GetName());
    if (pgProp) {
        if (event.GetProperty()->IsExpanded())
            m_pg->Expand(pgProp);
        else
            m_pg->Collapse(pgProp);
    }
}

void ObjectInspector::OnObjectSelected(wxWeaverObjectEvent& event)
{
    bool isForced = (event.GetString() == "force");
    Create(isForced);
}

void ObjectInspector::OnProjectRefresh(wxWeaverEvent&)
{
    Create(true);
}

void ObjectInspector::OnEventHandlerModified(wxWeaverEventHandlerEvent& event)
{
    PEvent e = event.GetWvrEventHandler();
    m_eg->SetPropertyValue(e->GetName(), e->GetValue());
    m_eg->Refresh();
}

void ObjectInspector::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    LogDebug(""); // TODO: ???
    PProperty prop = event.GetWvrProperty();
    PObjectBase propobj = prop->GetObject();
    PObjectBase appobj = AppData()->GetSelectedObject();

    bool shouldContinue = (prop->GetObject() == AppData()->GetSelectedObject());
    if (!shouldContinue) {
        // Item objects cannot be selected - their children are selected instead
        if (propobj->GetObjectInfo()->GetType()->IsItem()) {
            if (propobj->GetChildCount() > 0)
                shouldContinue = (appobj == propobj->GetChild(0));
        }
    }
    if (!shouldContinue)
        return;

    wxPGProperty* pgProp = m_pg->GetPropertyByLabel(prop->GetName());
    if (!pgProp)
        return; // Maybe now isn't showing this page

    switch (prop->GetType()) {
    case PT_FLOAT: {
        // Use float instead of string -> typeconv handles locale
        pgProp->SetValue(WXVARIANT(prop->GetValueAsFloat()));
        break;
    }
    case PT_INT:
    case PT_UINT: {
        pgProp->SetValueFromString(prop->GetValueAsString(), 0);
        break;
    }
    case PT_TEXT:
        pgProp->SetValueFromString(prop->GetValueAsString(), 0);
        break;
    case PT_MACRO:
    case PT_OPTION:
    case PT_EDIT_OPTION:
    case PT_PARENT:
    case PT_WXSTRING:
        pgProp->SetValueFromString(prop->GetValueAsText(), 0);
        break;
    case PT_WXSTRING_I18N:
        pgProp->SetValueFromString(prop->GetValueAsText(), 0);
        break;
    case PT_BOOL:
        pgProp->SetValueFromInt(prop->GetValueAsString() == "0" ? 0 : 1, 0);
        break;
    case PT_BITLIST: {
        wxString aux = prop->GetValueAsString();
        aux.Replace("|", ", ");
        if (aux == "0")
            aux = "";
        pgProp->SetValueFromString(aux, 0);
    } break;
    case PT_WXPOINT: {
        //m_pg->SetPropertyValue( pgProp, prop->GetValue() );
        wxString aux = prop->GetValueAsString();
        aux.Replace(",", ";");
        pgProp->SetValueFromString(aux, 0);
    } break;
    case PT_WXSIZE: {
        //m_pg->SetPropertyValue( pgProp, prop->GetValue() );
        wxString aux = prop->GetValueAsString();
        aux.Replace(",", ";");
        pgProp->SetValueFromString(aux, 0);
    } break;
    case PT_WXFONT:
        pgProp->SetValue(WXVARIANT(prop->GetValueAsString()));
        break;
    case PT_WXCOLOUR: {
        wxString value = prop->GetValueAsString();
        if (value.empty()) // Default Colour
        {
            wxColourPropertyValue def;
            def.m_type = wxSYS_COLOUR_WINDOW;
            def.m_colour = TypeConv::StringToSystemColour("wxSYS_COLOUR_WINDOW");
            m_pg->SetPropertyValue(pgProp, def);
        } else {
            if (!value.find_first_of("wx")) {
                wxColourPropertyValue def; // System Colour
                def.m_type = TypeConv::StringToSystemColour(value);
                def.m_colour = prop->GetValueAsColour();
                m_pg->SetPropertyValue(pgProp, WXVARIANT(def));
            } else {
                wxColourPropertyValue def(wxPG_COLOUR_CUSTOM, prop->GetValueAsColour());
                m_pg->SetPropertyValue(pgProp, WXVARIANT(def));
            }
        }
    } break;
    case PT_BITMAP:
        //      pgProp->SetValue( WXVARIANT( prop->GetValueAsString() ) );
        wxLogDebug("OI::OnPropertyModified: prop:%s", prop->GetValueAsString().c_str());
        break;
    default:
        pgProp->SetValueFromString(prop->GetValueAsString(), wxPG_FULL_VALUE);
    }
    AutoGenerateId(
        AppData()->GetSelectedObject(), event.GetWvrProperty(), "PropChange");

    m_pg->Refresh();
}

wxPropertyGridManager* ObjectInspector::CreatePropertyGridManager(wxWindow* parent,
                                                                  wxWindowID id)
{
    int pgStyle;
    int defaultDescBoxHeight;

    switch (m_style) {
    case wxWEAVER_OI_MULTIPAGE_STYLE:
        pgStyle = wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_DESCRIPTION
            | wxPG_TOOLBAR | wxPGMAN_DEFAULT_STYLE;
        defaultDescBoxHeight = 50;
        break;

    case wxWEAVER_OI_DEFAULT_STYLE:
    case wxWEAVER_OI_SINGLE_PAGE_STYLE:
    default:
        pgStyle = wxPG_BOLD_MODIFIED | wxPG_SPLITTER_AUTO_CENTER | wxPG_DESCRIPTION
            | wxPGMAN_DEFAULT_STYLE;
        defaultDescBoxHeight = 150;
        break;
    }
    int descBoxHeight;
    wxConfigBase* config = wxConfigBase::Get();
    config->Read(
        "/PropertyEditor/DescBoxHeight", &descBoxHeight, defaultDescBoxHeight);

    if (descBoxHeight == -1)
        descBoxHeight = defaultDescBoxHeight;

    wxPropertyGridManager* pg;
    pg = new wxPropertyGridManager(parent, id, wxDefaultPosition, wxDefaultSize, pgStyle);
    pg->SendSizeEvent();
    pg->SetDescBoxHeight(descBoxHeight);
#if 0
    // Both seems to no more needed.
    pg->SetExtraStyle( wxPG_EX_NATIVE_DOUBLE_BUFFERING );
    pg->SetExtraStyle( wxPG_EX_PROCESS_EVENTS_IMMEDIATELY );
#endif
    return pg;
}

void ObjectInspector::OnPropertyGridDblClick(wxPropertyGridEvent& event)
{
    PObjectBase obj = AppData()->GetSelectedObject();
    if (obj) {
        wxString propName = event.GetProperty()->GetLabel();
        AutoGenerateId(obj, obj->GetProperty(propName), "DblClk");
        m_pg->Refresh();
    }
}

void ObjectInspector::OnEventGridDblClick(wxPropertyGridEvent& event)
{
    wxPGProperty* pgProp = m_pg->GetPropertyByLabel("name");
    if (!pgProp)
        return;

    wxPGProperty* p = event.GetProperty();
    p->SetValueFromString(
        pgProp->GetDisplayedString() + event.GetProperty()->GetLabel());

    ObjInspectorEventMap::iterator it = m_eventMap.find(p);
    if (it != m_eventMap.end()) {
        PEvent evt = it->second;
        wxString handler = p->GetValueAsString();
        handler.Trim();
        handler.Trim(false);
        AppData()->ModifyEventHandler(evt, handler);
    };
}

void ObjectInspector::AutoGenerateId(PObjectBase objectChanged,
                                     PProperty propChanged, wxString reason)
{
    if (objectChanged && propChanged) {
        PProperty prop;
        if ((propChanged->GetName() == "name" && reason == "PropChange")
            || (propChanged->GetName() == "id" && reason == "DblClk")) {
#if 0
            wxPGId pgid = m_pg->GetPropertyByLabel(""); // Old code
#endif
            prop = AppData()->GetProjectData()->GetProperty("event_generation");
            if (prop) {
                if (prop->GetValueAsString() == "table") {
                    prop = objectChanged->GetProperty("id");
                    if (prop) {
                        if (prop->GetValueAsString() == "wxID_ANY"
                            || reason == "DblClk") {
                            PProperty name(objectChanged->GetProperty("name"));
                            wxString idString;
                            idString << "ID_";
                            idString << name->GetValueAsString().Upper();
                            ModifyProperty(prop, idString);

                            wxPGProperty* pgid = m_pg->GetPropertyByLabel("id");
                            if (!pgid)
                                return;

                            m_pg->SetPropertyValue(pgid, idString);
                        }
                    }
                } else {
                    prop = objectChanged->GetProperty("id");
                    if (prop) {
                        ModifyProperty(prop, "wxID_ANY");
                        wxPGProperty* pgid = m_pg->GetPropertyByLabel("id");
                        if (!pgid)
                            return;
                        m_pg->SetPropertyValue(pgid, "wxID_ANY");
                    }
                }
            }
        }
    }
    m_pg->Update();
}

void ObjectInspector::OnBitmapPropertyChanged(wxCommandEvent& event)
{
    wxLogDebug("OI::BitmapPropertyChanged: %s", event.GetString().c_str());

    wxString propName = event.GetString().BeforeFirst(':');
    wxString propVal = event.GetString().AfterFirst(':');

    if (!propVal.IsEmpty()) {
        wxWeaverBitmapProperty* bp
            = wxDynamicCast(m_pg->GetPropertyByLabel(propName), wxWeaverBitmapProperty);
        if (bp)
            bp->UpdateChildValues(propVal);
    }
}

void ObjectInspector::ModifyProperty(PProperty prop, const wxString& str)
{
    AppData()->RemoveHandler(this->GetEventHandler());
    AppData()->ModifyProperty(prop, str);
    AppData()->AddHandler(this->GetEventHandler());
}

void ObjectInspector::OnChildFocus(wxChildFocusEvent&)
{
    // TODO: do nothing to avoid "scrollbar jump" if wx2.9+ is used
}

void ObjectInspector::OnPropertyGridItemSelected(wxPropertyGridEvent& event)
{
    wxPGProperty* p = event.GetProperty();
    if (p) {
        if (!m_nb->GetSelection()) {
            m_strSelPropItem = m_pg->GetPropertyName(p);
            m_pageName = _("Properties");
        } else {
            m_strSelPropItem = m_eg->GetPropertyName(p);
            m_pageName = _("Events");
        }
    }
}

void ObjectInspector::RestoreLastSelectedPropItem()
{
    if (m_pageName == _("Properties")) {
        wxPGProperty* p = m_pg->GetPropertyByName(m_strSelPropItem);
        if (p) {
            m_pg->SelectProperty(p, true);
            m_pg->SetFocus();
        }
    } else if (m_pageName == _("Events")) {
        wxPGProperty* p = m_eg->GetPropertyByName(m_strSelPropItem);
        if (p) {
            m_eg->SelectProperty(p, true);
            m_eg->SetFocus();
        }
    }
}
