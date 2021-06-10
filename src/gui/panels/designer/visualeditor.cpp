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
#include "gui/panels/designer/visualeditor.h"

#include "model/objectbase.h"
#include "utils/typeconv.h"
#include "utils/exception.h"
#include "appdata.h"
#include "event.h"
#include "manager.h"
#include "gui/panels/designer/menubar.h"

#include <wx/collpane.h>

VisualEditor::VisualEditor(wxWindow* parent)
    : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
    , m_auiMgr(nullptr)
    , m_wizard(nullptr)
    , m_designer(new DesignerWindow(this, wxID_ANY, wxPoint(10, 10)))
    , m_auiPanel(nullptr)
    , m_stopSelectedEvent(false)
    , m_stopModifiedEvent(false)

{
    SetExtraStyle(wxWS_EX_BLOCK_EVENTS);

    AppData()->AddHandler(this->GetEventHandler());

    if (!AppData()->IsDarkMode()) {
        SetOwnBackgroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE).ChangeLightness(80));
    } else {
        SetOwnBackgroundColour(
            wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE).ChangeLightness(120));
    }
    SetScrollRate(5, 5);

    m_auiScanTimer.SetOwner(this); // scan aui panes at run-time
    m_auiScanTimer.Start(200);

#if 0
    Bind(wxEVT_WVR_INNER_FRAME_RESIZED, &VisualEditor::OnResizeBackPanel, this);
#endif
    Bind(wxEVT_WVR_PROJECT_LOADED, &VisualEditor::OnProjectLoaded, this);
    Bind(wxEVT_WVR_PROJECT_SAVED, &VisualEditor::OnProjectSaved, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &VisualEditor::OnProjectRefresh, this);
    Bind(wxEVT_WVR_CODE_GENERATION, &VisualEditor::OnProjectRefresh, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &VisualEditor::OnObjectSelected, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &VisualEditor::OnObjectCreated, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &VisualEditor::OnObjectRemoved, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &VisualEditor::OnPropertyModified, this);
    Bind(wxEVT_TIMER, &VisualEditor::OnAuiScan, this);

    m_designer->Bind(wxEVT_LEFT_DOWN, &VisualEditor::OnClickBackPanel, this);
}

void VisualEditor::DeleteAbstractObjects()
{
    for (wxObjectMap::iterator it = m_wxobjects.begin();
         it != m_wxobjects.end(); ++it) {

        // The abstract objects are stored as wxNoObject*'s
        wxNoObject* noobject = dynamic_cast<wxNoObject*>(it->first);
        if (noobject) {
            delete noobject;
        } else {
            // Delete push'd visual object event handlers
            wxStaticBoxSizer* staticBoxSizer = wxDynamicCast(it->first, wxStaticBoxSizer);
            if (staticBoxSizer) {
                staticBoxSizer->GetStaticBox()->PopEventHandler(true);
                return;
            }
            wxWindow* window = wxDynamicCast(it->first, wxWindow);
            if (window)
                window->PopEventHandler(true);
        }
    }
}

VisualEditor::~VisualEditor()
{
    m_auiScanTimer.Stop();

    AppData()->RemoveHandler(this->GetEventHandler());
    DeleteAbstractObjects();

    ClearAui();
    ClearWizard();
    ClearComponents(m_designer->GetFrameContentPanel());
}

void VisualEditor::UpdateVirtualSize()
{
    int w, h, panelW, panelH;
    GetVirtualSize(&w, &h);
    m_designer->GetSize(&panelW, &panelH);
    panelW += 20;
    panelH += 20;
    if (panelW != w || panelH != h)
        SetVirtualSize(panelW, panelH);
}

void VisualEditor::OnClickBackPanel(wxMouseEvent& event)
{
    if (m_form)
        AppData()->SelectObject(m_form);

    event.Skip();
}

void VisualEditor::OnResizeBackPanel(wxCommandEvent& /*event*/)
{
    LogDebug("event: OnResizeBackPanel");

    PObjectBase form(AppData()->GetSelectedForm());
    if (!form)
        return;

    PProperty sizeProperty(form->GetProperty("size"));
    if (!sizeProperty)
        return;

    wxString value(TypeConv::PointToString(
        wxPoint(m_designer->GetSize().x, m_designer->GetSize().y)));

    AppData()->ModifyProperty(sizeProperty, value);
}

void VisualEditor::OnWizardPageChanged(WizardEvent& event)
{
    if (!event.GetInt()) {
        AppData()->GetManager()->SelectObject(event.GetPage());
        return;
    }
    event.Skip();
}

PObjectBase VisualEditor::GetObjectBase(wxObject* wxobject)
{
    if (!wxobject) {
        wxLogError("wxobject is a nullptr!");
        return PObjectBase();
    }
    wxObjectMap::iterator obj = m_wxobjects.find(wxobject);
    if (obj != m_wxobjects.end()) {
        return obj->second;
    } else {
        wxLogError("No corresponding ObjectBase for wxObject. Name: %s",
                   wxobject->GetClassInfo()->GetClassName());
        return PObjectBase();
    }
}

wxObject* VisualEditor::GetWxObject(PObjectBase baseobject)
{
    if (!baseobject) {
        wxLogError("baseobject is a nullptr!");
        return nullptr;
    }
    ObjectBaseMap::iterator obj = m_baseobjects.find(baseobject.get());
    if (obj != m_baseobjects.end()) {
        return obj->second;
    } else {
        wxLogError("No corresponding wxObject for ObjectBase. Name: %s",
                   baseobject->GetClassName().c_str());
        return nullptr;
    }
}

void VisualEditor::ScanPanes(wxWindow* parent)
{
    bool updateNeeded;
    wxLogNull stopTheLogging;
    const wxWindowList& children = parent->GetChildren();

    for (wxWindowList::const_reverse_iterator child = children.rbegin();
         child != children.rend(); ++child) {
        ScanPanes(*child);

        PObjectBase obj = GetObjectBase(*child);
        if (!obj)
            continue;

        updateNeeded = false;
        wxString objTypeName = obj->GetObjectInfo()->GetType()->GetName();
        if (objTypeName == "widget"
            || objTypeName == "expanded_widget"
            || objTypeName == "ribbonbar"
            || objTypeName == "propgrid"
            || objTypeName == "propgridman"
            || objTypeName == "dataviewctrl"
            || objTypeName == "dataviewtreectrl"
            || objTypeName == "dataviewlistctrl"
            || objTypeName == "toolbar"
            || objTypeName == "container") {
            wxAuiPaneInfo inf = m_auiMgr->GetPane(*child);
            if (inf.IsOk()) {
                // scan position and docking mode
                if (!obj->GetPropertyAsInteger("center_pane")) {
                    wxString dock;
                    if (inf.IsDocked()) {
                        wxString dockDir;
                        switch (inf.dock_direction) {
                        case 1:
                            dockDir = "Top";
                            break;

                        case 2:
                            dockDir = "Right";
                            break;

                        case 3:
                            dockDir = "Bottom";
                            break;

                        case 4:
                            dockDir = "Left";
                            break;

                        case 5:
                            dockDir = "Center";
                            break;

                        default:
                            dockDir = "Left";
                            break;
                        }
                        PProperty pdock = obj->GetProperty("docking");
                        if (pdock->GetValue() != dockDir) {
                            pdock->SetValue(dockDir);
                            updateNeeded = true;
                        }
                        dock = "Dock";
                    } else {
                        wxPoint pos = inf.floating_pos;
                        if (pos.x != -1 && pos.y != -1) {
                            PProperty pposition = obj->GetProperty("pane_position");
                            if (pposition->GetValue() != TypeConv::PointToString(pos)) {
                                pposition->SetValue(TypeConv::PointToString(pos));
                                updateNeeded = true;
                            }
                        }
                        wxSize paneSize = inf.floating_size;
                        if (paneSize.x != -1 && paneSize.y != -1) {
                            PProperty psize = obj->GetProperty("pane_size");
                            if (psize->GetValue() != TypeConv::SizeToString(paneSize)) {
                                psize->SetValue(TypeConv::SizeToString(paneSize));
                                obj->GetProperty("resize")->SetValue(wxS("Resizable"));
                                updateNeeded = true;
                            }
                        }
                        dock = "Float";
                    }
                    PProperty pfloat = obj->GetProperty("dock");
                    if (pfloat->GetValue() != dock) {
                        pfloat->SetValue(dock);
                        updateNeeded = true;
                    }
#if 0
                    // scan "best size"
                    wxSize bestSize = inf.best_size;
                    if (bestSize.x != -1 && bestSize.y != -1) {
                        PProperty psize = obj->GetProperty("best_size");
                        if (psize->GetValue() != TypeConv::SizeToString(bestSize)) {
                            psize->SetValue(TypeConv::SizeToString(bestSize));
                            obj->GetProperty("resize")->SetValue(wxS("Resizable"));
                            updateNeeded = true;
                        }
                    }
#endif
                    PProperty prop = obj->GetProperty("aui_row");
                    if (obj->GetPropertyAsInteger("aui_row") != inf.dock_row) {
                        prop->SetValue(inf.dock_row);
                        updateNeeded = true;
                    }
                    prop = obj->GetProperty("aui_layer");
                    if (obj->GetPropertyAsInteger("aui_layer") != inf.dock_layer) {
                        prop->SetValue(inf.dock_layer);
                        updateNeeded = true;
                    }
                }
                PProperty pshow = obj->GetProperty("show");
                if (obj->GetPropertyAsInteger("show") != (int)inf.IsShown()) {
                    pshow->SetValue(inf.IsShown() ? 1 : 0);
                    updateNeeded = true;
                }
                if (updateNeeded)
                    AppData()->SelectObject(obj, true, true);
            }
        }
    }
}

void VisualEditor::ClearAui()
{
    if (m_auiMgr) {
        m_auiMgr->UnInit();

        delete m_auiMgr;
        m_auiMgr = nullptr;
        m_auiPanel = nullptr;
    }
}

void VisualEditor::ClearWizard()
{
    if (m_wizard) {
        m_wizard->Unbind(wxEVT_WVR_WIZARD_PAGE_CHANGED,
                         &VisualEditor::OnWizardPageChanged, this);
        m_wizard->Destroy();
        m_wizard = nullptr;
    }
}

void VisualEditor::ClearComponents(wxWindow* parent)
{
    /*
        Individual wxWindow's of composite components made of wxWindow's
        will be found here as well and won't have an associated ObjectBase
    */
    wxLogNull stopTheLogging; // prevent the error log messages
    const wxWindowList& children = parent->GetChildren();
    for (wxWindowList::const_reverse_iterator child = children.rbegin();
         child != children.rend(); ++child) {
        ClearComponents(*child);

        PObjectBase obj = GetObjectBase(*child);
        if (!obj)
            continue;

        PObjectInfo objInfo = obj->GetObjectInfo();
        IComponent* comp = objInfo->GetComponent();
        if (comp)
            comp->Cleanup(*child);
    }
}

void VisualEditor::Create()
{
#if !defined(__WXGTK__)
    if (IsShown()) {
        Freeze(); // Prevented flickering on wx 2.8,
                  // caused problems on wxGTK 2.9 (e.g. wxNoteBook objects)
    }
#endif
    DeleteAbstractObjects(); // Delete objects which had no parent

    m_designer->SetSelectedItem(nullptr); // Clear selections, delete objects
    m_designer->SetSelectedSizer(nullptr);
    m_designer->SetSelectedObject(PObjectBase());

    ClearAui();
    ClearWizard();
    ClearComponents(m_designer->GetFrameContentPanel());

    m_designer->GetFrameContentPanel()->DestroyChildren();
    m_designer->GetFrameContentPanel()->SetSizer(nullptr); // *!* <-- TODO: ???

    m_wxobjects.clear(); // Clear all associations between ObjectBase and wxObjects
    m_baseobjects.clear();

    if (IsShown()) {
        m_form = AppData()->GetSelectedForm();
        if (m_form) {
            m_designer->Show(true);

            // --- [1] Configure the size of the form ---------------------------

            // Get size properties
            wxSize minSize(m_form->GetPropertyAsSize("minimum_size"));
            m_designer->SetMinSize(minSize);

            wxSize maxSize(m_form->GetPropertyAsSize("maximum_size"));
            m_designer->SetMaxSize(maxSize);

            wxSize size(m_form->GetPropertyAsSize("size"));

            // Determine necessary size for back panel
            wxSize backSize = size;
            if (backSize.GetWidth() < minSize.GetWidth()
                && backSize.GetWidth() != wxDefaultCoord) {
                backSize.SetWidth(minSize.GetWidth());
            }
            if (backSize.GetHeight() < minSize.GetHeight()
                && backSize.GetHeight() != wxDefaultCoord) {
                backSize.SetHeight(minSize.GetHeight());
            }
            if (backSize.GetWidth() > maxSize.GetWidth()
                && maxSize.GetWidth() != wxDefaultCoord) {
                backSize.SetWidth(maxSize.GetWidth());
            }
            if (backSize.GetHeight() > maxSize.GetHeight()
                && maxSize.GetHeight() != wxDefaultCoord) {
                backSize.SetHeight(maxSize.GetHeight());
            }
            if (size != backSize) {
                /*
                    Since this could be called by VisualEditor::OnPropertyModified
                    we must not trigger a modify event again.
                    Creating a delayed event won't work either, as this would
                    mess up the undo/redo stack.
                    Therefore we just log about the invalid size:
                */
                LogDebug("size is NOT between minimum_size and maximum_size");
            }

            // --- [2] Set the color of the form -------------------------------

            PProperty background(m_form->GetProperty("bg"));
            if (background && !background->GetValue().empty()) {
                m_designer->GetFrameContentPanel()->SetBackgroundColour(
                    TypeConv::StringToColour(background->GetValue()));
            } else {
                if (m_form->GetClassName() == "Frame") {
                    m_designer->GetFrameContentPanel()->SetOwnBackgroundColour(
                        wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE));
                } else {
#ifdef __WXGTK__
                    wxVisualAttributes attribs = wxToolBar::GetClassDefaultAttributes();
                    m_designer->GetFrameContentPanel()->SetOwnBackgroundColour(attribs.colBg);
#else
                    m_designer->GetFrameContentPanel()->SetOwnBackgroundColour(
                        wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
#endif
                }
            }
            // --- [3] Title bar Setup

            if (m_form->GetClassName() == "Frame" // TODO: isTopLevel()
                || m_form->GetClassName() == "Dialog"
                || m_form->GetClassName() == "Wizard") {
                long style = m_form->GetPropertyAsInteger("style");
                m_designer->SetTitleStyle(style);
                m_designer->SetTitle(m_form->GetPropertyAsString("title"));
                m_designer->ShowTitleBar((style & wxCAPTION));
            } else {
                m_designer->ShowTitleBar(false);
            }
            // --- AUI
            if (m_form->GetTypeName() == "form") {
                if (m_form->GetPropertyAsInteger("aui_managed")) {
                    m_auiPanel = new wxPanel(m_designer->GetFrameContentPanel());
                    m_auiMgr = new wxAuiManager(
                        m_auiPanel,
                        m_form->GetPropertyAsInteger("aui_manager_style"));
                }
            }
            // --- Wizard
            if (m_form->GetClassName() == "Wizard") {
                m_wizard = new Wizard(m_designer->GetFrameContentPanel());

                bool showbutton = false;
                PProperty extraStyle = m_form->GetProperty("extra_style");
                if (extraStyle)
                    showbutton = extraStyle->GetValue().Contains("wxWIZARD_EX_HELPBUTTON");

                m_wizard->ShowHelpButton(showbutton);

                if (!m_form->GetProperty("bitmap")->IsNull()) {
                    wxBitmap bmp = m_form->GetPropertyAsBitmap("bitmap");
                    if (bmp.IsOk())
                        m_wizard->SetBitmap(bmp);
                }
            }
            // --- [4] Create the components of the form -------------------------

            // Used to save frame objects for later display
            PObjectBase menubar;
            wxWindow* statusbar = nullptr;
            wxWindow* toolbar = nullptr;

            for (size_t i = 0; i < m_form->GetChildCount(); i++) {
                PObjectBase child = m_form->GetChild(i);

                if (!menubar && (m_form->GetTypeName() == "menubar_form")) {
                    // main form acts as a menubar
                    menubar = m_form;
                } else if (child->GetTypeName() == "menubar") {
                    // Create the menubar later
                    menubar = child;
                } else if (!toolbar && m_form->GetTypeName() == "toolbar_form") {
                    Generate(m_form,
                             m_designer->GetFrameContentPanel(),
                             m_designer->GetFrameContentPanel());

                    ObjectBaseMap::iterator it = m_baseobjects.find(m_form.get());
                    toolbar = wxDynamicCast(it->second, wxToolBar);
                    break;
                } else {
                    try { // Recursively generate the ObjectTree
                        // we have to put the content frame panel as parentObject
                        // in order to SetSizeHints be called.
                        if (m_auiPanel) {
                            Generate(child, m_auiPanel, m_auiPanel);
                        } else if (m_wizard) {
                            Generate(child, m_wizard, m_wizard);
                        } else {
                            Generate(child,
                                     m_designer->GetFrameContentPanel(),
                                     m_designer->GetFrameContentPanel());
                        }
                    } catch (wxWeaverException& ex) {
                        wxLogError(ex.what());
                    }
                }
                // Attach the toolbar (if any) to the frame
                if (child->GetClassName() == "wxToolBar") {
                    ObjectBaseMap::iterator it = m_baseobjects.find(child.get());
                    toolbar = wxDynamicCast(it->second, wxToolBar);
                } else if (child->GetClassName() == "wxAuiToolBar") {
                    ObjectBaseMap::iterator it = m_baseobjects.find(child.get());
                    toolbar = wxDynamicCast(it->second, wxAuiToolBar);
                }
                // Attach the status bar (if any) to the frame
                if (child->GetObjectInfo()->IsSubclassOf("wxStatusBar")) {
                    ObjectBaseMap::iterator it = m_baseobjects.find(child.get());
                    statusbar = wxDynamicCast(it->second, wxStatusBar);
                }
                // Add toolbar(s) to AuiManager and update content
                if (m_auiMgr && toolbar) {
                    SetupAui(GetObjectBase(toolbar), toolbar);
                    toolbar = nullptr;
                }
            }
            if (menubar || statusbar || toolbar || m_auiPanel || m_wizard) {
                if (m_auiMgr) {
                    m_designer->SetFrameWidgets(menubar, nullptr, statusbar, m_auiPanel);
                } else if (m_wizard) {
                    m_designer->SetFrameWidgets(menubar, nullptr, nullptr, m_wizard);
                } else
                    m_designer->SetFrameWidgets(menubar, toolbar, statusbar, m_auiPanel);
            }
            m_designer->Layout();

            if (backSize.GetHeight() == wxDefaultCoord
                || backSize.GetWidth() == wxDefaultCoord) {
                m_designer->GetSizer()->Fit(m_designer);
                m_designer->SetClientSize(m_designer->GetBestSize());
            }
            // Set size after fitting so if only one dimesion is -1,
            // it still fits that dimension
            m_designer->SetSize(backSize);

            PProperty enabled(m_form->GetProperty("enabled"));
            if (enabled)
                m_designer->Enable(TypeConv::StringToInt(enabled->GetValue()));

            PProperty hidden(m_form->GetProperty("hidden"));
            if (hidden)
                m_designer->Show(!TypeConv::StringToInt(hidden->GetValue()));

            if (m_auiMgr)
                m_auiMgr->Update();
            else
                m_designer->Refresh();

            Refresh();
        } else {
            m_designer->Show(false); // There is no form to display
            Refresh();
        }
#if !defined(__WXGTK__)
        Thaw();
#endif
    }
    UpdateVirtualSize();
}

void VisualEditor::Generate(PObjectBase obj, wxWindow* wxparent,
                            wxObject* parentObject)
{
    PObjectInfo objInfo = obj->GetObjectInfo();
    IComponent* comp = objInfo->GetComponent();

    if (!comp) {
        wxWEAVER_THROW_EX(wxString::Format(
            "Component for %s not found!", obj->GetClassName().c_str()));
    }
    wxObject* createdObject = comp->Create(obj.get(), wxparent);
    wxWindow* createdWindow = nullptr;
    wxSizer* createdSizer = nullptr;
    wxWindow* vobjWindow = nullptr;
    wxEvtHandler* vobjHandler = nullptr;

    switch (comp->GetComponentType()) {
    case ComponentType::Window:
        createdWindow = wxDynamicCast(createdObject, wxWindow);
        if (!createdWindow) {
            wxWEAVER_THROW_EX(wxString::Format(
                "Component for %s was registered as a window component, but this is not a wxWindow!",
                obj->GetClassName().c_str()));
        }
        SetupWindow(obj, createdWindow);
        /*
            TODO: rephrase this:
            [*] The event handler must be pushed after OnCreated() because
            that might push its own event handlers, so record it here only
            Because wxCollapsiblePane replaces createdWindow the target
            for the event handler must be recorded as well
        */
        vobjWindow = createdWindow;
        vobjHandler = new VObjEvtHandler(createdWindow, obj);
        break;

    case ComponentType::Sizer: {
        wxStaticBoxSizer* staticBoxSizer = wxDynamicCast(createdObject, wxStaticBoxSizer);
        if (staticBoxSizer) {
            createdWindow = staticBoxSizer->GetStaticBox();
            createdSizer = staticBoxSizer;
        } else {
            createdSizer = wxDynamicCast(createdObject, wxSizer);
        }
        if (!createdSizer) {
            wxWEAVER_THROW_EX(wxString::Format(
                "Component for %s was registered as a sizer component, but this is not a wxSizer!",
                obj->GetClassName().c_str()));
        }
        SetupSizer(obj, createdSizer);

        if (createdWindow) {
            // [*]
            vobjWindow = createdWindow;
            vobjHandler = new VObjEvtHandler(createdWindow, obj);
        }
        break;
    }
    default:
        break;
    }

    // Associate the wxObject* with the PObjectBase
    m_wxobjects.insert(wxObjectMap::value_type(createdObject, obj));
    m_baseobjects.insert(ObjectBaseMap::value_type(obj.get(), createdObject));

    // Access to collapsible pane
    wxCollapsiblePane* collpane = wxDynamicCast(createdObject, wxCollapsiblePane);
    if (collpane) {
        createdWindow = collpane->GetPane();
        createdObject = createdWindow;
    }
    // New wxparent for the window's children
    wxWindow* new_wxparent = (createdWindow ? createdWindow : wxparent);

    // Recursively generate the children
    for (size_t i = 0; i < obj->GetChildCount(); i++)
        Generate(obj->GetChild(i), new_wxparent, createdObject);

    comp->OnCreated(createdObject, wxparent);

    // Now push the event handler so that it will be the last one in the chain
    if (vobjWindow && vobjHandler)
        vobjWindow->PushEventHandler(vobjHandler);

    // If the created object is a sizer and the parent object is a window,
    // set the sizer to the window
    if ((createdSizer && wxDynamicCast(parentObject, wxWindow))
        || (!parentObject && createdSizer)) {
        wxparent->SetSizer(createdSizer);
        if (parentObject)
            createdSizer->SetSizeHints(wxparent);

        wxparent->SetAutoLayout(true);
        wxparent->Layout();
    }
}

void VisualEditor::SetupSizer(PObjectBase obj, wxSizer* sizer)
{
    wxSize minsize = obj->GetPropertyAsSize("minimum_size");
    if (minsize != wxDefaultSize) {
        sizer->SetMinSize(minsize);
        sizer->Layout();
    }
}

void VisualEditor::SetupWindow(PObjectBase obj, wxWindow* window)
{
    // All of the properties of the wxWindow object are applied in this function
#if 0
    // Position does nothing in wxWeaver, this is pointless
    wxPoint pos;
    PProperty posProperty = obj->GetProperty("pos");
    if (posProperty)
        pos = TypeConv::StringToPoint(posProperty->GetValue());
#endif
    // Size
    wxSize size = obj->GetPropertyAsSize("size");
    if (size != wxDefaultSize)
        window->SetSize(size);

    // Minimum size
    wxSize minSize = obj->GetPropertyAsSize("minimum_size");
    if (minSize != wxDefaultSize)
        window->SetMinSize(minSize);

    // Maximum size
    wxSize maxSize = obj->GetPropertyAsSize("maximum_size");
    if (maxSize != wxDefaultSize)
        window->SetMaxSize(maxSize);

    // Font
    PProperty fontProperty = obj->GetProperty("font");
    if (fontProperty && !fontProperty->GetValue().empty())
        window->SetFont(TypeConv::StringToFont(fontProperty->GetValue()));

    // Foreground
    PProperty fgProperty = obj->GetProperty("fg");
    if (fgProperty && !fgProperty->GetValue().empty())
        window->SetForegroundColour(TypeConv::StringToColour(fgProperty->GetValue()));

    // Background
    PProperty bgProperty = obj->GetProperty("bg");
    if (bgProperty && !bgProperty->GetValue().empty())
        window->SetBackgroundColour(TypeConv::StringToColour(bgProperty->GetValue()));

    // Extra Style
    PProperty exStyleProperty = obj->GetProperty("window_extra_style");
    if (exStyleProperty)
        window->SetExtraStyle(TypeConv::StringToInt(exStyleProperty->GetValue()));

    // Enabled
    PProperty enabledProperty = obj->GetProperty("enabled");
    if (enabledProperty)
        window->Enable((enabledProperty->GetValueAsInteger()));

    // Hidden
    PProperty hiddenProperty = obj->GetProperty("hidden");
    if (hiddenProperty)
        window->Show(!hiddenProperty->GetValueAsInteger());

    // Tooltip
    PProperty tooltipProperty = obj->GetProperty("tooltip");
    if (tooltipProperty)
        window->SetToolTip(tooltipProperty->GetValueAsString());

    // AUI
    // clang-format off
    wxString objTypeName = obj->GetObjectInfo()->GetType()->GetName();
    if (m_auiMgr &&
          (objTypeName == "widget"
        || objTypeName == "expanded_widget"
        || objTypeName == "container"
        || objTypeName == "notebook"
        || objTypeName == "auinotebook"
        || objTypeName == "listbook"
        || objTypeName == "simplebook"
        || objTypeName == "choicebook"
        || objTypeName == "treelistctrl"
        || objTypeName == "ribbonbar"
        || objTypeName == "dataviewctrl"
        || objTypeName == "dataviewtreectrl"
        || objTypeName == "dataviewlistctrl"
        || objTypeName == "propgrid"
        || objTypeName == "propgridman"
        || objTypeName == "splitter"))
    {
        if (obj->GetParent()->GetTypeName() == "form")
            SetupAui(obj, window);
    }
    else if (obj->GetParent()->GetTypeName() == "wizard")
    {
        SetupWizard(obj, window, true);
    }
    // clang-format on
}

void VisualEditor::SetupAui(PObjectBase obj, wxWindow* window)
{
    wxAuiPaneInfo info;

    // check whether the object contains AUI info...
    if (!obj->GetProperty("aui_name"))
        return;

    wxString name = obj->GetPropertyAsString("aui_name");
    if (name != "")
        info.Name(name);

    if (obj->GetPropertyAsInteger("center_pane"))
        info.CenterPane();

    if (obj->GetPropertyAsInteger("default_pane"))
        info.DefaultPane();

    if (!obj->IsNull("caption"))
        info.Caption(obj->GetPropertyAsString("caption"));

    info.CaptionVisible(obj->GetPropertyAsInteger("caption_visible"));
    info.CloseButton(obj->GetPropertyAsInteger("close_button"));
    info.MaximizeButton(obj->GetPropertyAsInteger("maximize_button"));
    info.MinimizeButton(obj->GetPropertyAsInteger("minimize_button"));
    info.PinButton(obj->GetPropertyAsInteger("pin_button"));
    info.PaneBorder(obj->GetPropertyAsInteger("pane_border"));
    info.Gripper(obj->GetPropertyAsInteger("gripper"));

    info.BottomDockable(obj->GetPropertyAsInteger("BottomDockable"));
    info.TopDockable(obj->GetPropertyAsInteger("TopDockable"));
    info.LeftDockable(obj->GetPropertyAsInteger("LeftDockable"));
    info.RightDockable(obj->GetPropertyAsInteger("RightDockable"));

    if (!obj->IsNull("dock")) {
        if (obj->GetPropertyAsString("dock") == "Dock") {
            info.Dock();
            if (!obj->IsNull("docking")) {
                if (obj->GetPropertyAsString("docking") == "Bottom")
                    info.Bottom();
                else if (obj->GetPropertyAsString("docking") == "Top")
                    info.Top();
                else if (obj->GetPropertyAsString("docking") == "Center")
                    info.Center();
                else if (obj->GetPropertyAsString("docking") == "Right")
                    info.Right();
            }
        } else {
            info.Float();
            info.FloatingPosition(obj->GetPropertyAsPoint("pane_position"));
        }
    }
    if (!obj->IsNull("resize")) {
        if (obj->GetPropertyAsString("resize") == "Resizable")
            info.Resizable();
        else
            info.Fixed();
    }
    info.DockFixed(obj->GetPropertyAsInteger("dock_fixed"));
    info.Movable(obj->GetPropertyAsInteger("moveable"));
    info.Floatable(obj->GetPropertyAsInteger("floatable"));

    if (!obj->GetProperty("pane_size")->IsNull())
        info.FloatingSize(obj->GetPropertyAsSize("pane_size"));

    if (!obj->GetProperty("best_size")->IsNull())
        info.BestSize(obj->GetPropertyAsSize("best_size"));

    if (!obj->GetProperty("min_size")->IsNull())
        info.MinSize(obj->GetPropertyAsSize("min_size"));

    if (!obj->GetProperty("max_size")->IsNull())
        info.MaxSize(obj->GetPropertyAsSize("max_size"));

    if (obj->GetPropertyAsInteger("toolbar_pane"))
        info.ToolbarPane();

    if (!obj->IsNull("aui_position"))
        info.Position(obj->GetPropertyAsInteger("aui_position"));

    if (!obj->IsNull("aui_row"))
        info.Row(obj->GetPropertyAsInteger("aui_row"));

    if (!obj->IsNull("aui_layer"))
        info.Layer(obj->GetPropertyAsInteger("aui_layer"));

    if (!obj->GetPropertyAsInteger("show"))
        info.Hide();

    m_auiMgr->AddPane(window, info);
}

void VisualEditor::SetupWizard(PObjectBase obj, wxWindow* window, bool pageAdding)
{
    WizardPageSimple* wizPage = wxDynamicCast(window, WizardPageSimple);

    if (pageAdding) {
        m_wizard->AddPage(wizPage);
        m_wizard->Bind(wxEVT_WVR_WIZARD_PAGE_CHANGED,
                       &VisualEditor::OnWizardPageChanged, this);
    } else {
        WizardEvent eventChanged(
            wxEVT_WVR_WIZARD_PAGE_CHANGED, m_wizard->GetId(), false, wizPage);

        eventChanged.SetInt(1);
        wizPage->GetEventHandler()->ProcessEvent(eventChanged);

        bool wizBmpOk = !obj->GetParent()->GetProperty("bitmap")->IsNull();
        bool pgeBmpOk = !obj->GetProperty("bitmap")->IsNull();
        wxBitmap wizBmp = obj->GetParent()->GetPropertyAsBitmap("bitmap");
        wxBitmap pgeBmp = obj->GetPropertyAsBitmap("bitmap");

        if (pgeBmpOk && pgeBmp.IsOk())
            m_wizard->SetBitmap(pgeBmp);
        else if (wizBmpOk && wizBmp.IsOk())
            m_wizard->SetBitmap(wizBmp);

        size_t selection = m_wizard->GetPageIndex(wizPage);
        m_wizard->SetSelection(selection);
    }
}

void VisualEditor::PreventOnSelected(bool prevent)
{
    m_stopSelectedEvent = prevent;
}

void VisualEditor::PreventOnModified(bool prevent)
{
    m_stopModifiedEvent = prevent;
}

void VisualEditor::OnProjectLoaded(wxWeaverEvent&)
{
    Create();
}

void VisualEditor::OnProjectSaved(wxWeaverEvent&)
{
#if 0
    Create();
#endif
}

void VisualEditor::OnObjectSelected(wxWeaverObjectEvent& event)
{
    // It is only necessary to Create() if the selected object is on a different form
    if (AppData()->GetSelectedForm() != m_form)
        Create();

    // Get the ObjectBase from the event
    PObjectBase obj = event.GetWvrObject();
    if (!obj) {
        LogDebug("The event object is a nullptr");
        return;
    }
    // highlight parent toolbar instead of its children
    PObjectBase toolbar = obj->FindNearAncestor("toolbar");
    if (toolbar)
        obj = toolbar;
    else
        toolbar = obj->FindNearAncestor("toolbar_form");

    // Make sure this is a visible object
    ObjectBaseMap::iterator it = m_baseobjects.find(obj.get());
    if (it == m_baseobjects.end()) {
        m_designer->SetSelectedSizer(nullptr);
        m_designer->SetSelectedItem(nullptr);
        m_designer->SetSelectedObject(PObjectBase());
        m_designer->SetSelectedPanel(nullptr);
        m_designer->Refresh();
        return;
    }
    wxObject* item = it->second; // Save wxobject
    ComponentType componentType = ComponentType::Abstract;

    IComponent* comp = obj->GetObjectInfo()->GetComponent();
    if (comp) {
        componentType = comp->GetComponentType();

        if (!m_stopSelectedEvent)
            comp->OnSelected(item); // Fire selection event in plugin
    }
    if (obj->GetObjectInfo()->GetTypeName() == "wizardpagesimple") {
        ObjectBaseMap::iterator pageIt = m_baseobjects.find(obj.get());
        WizardPageSimple* wizpage = wxDynamicCast(pageIt->second, WizardPageSimple);
        SetupWizard(obj, wizpage);
    }
    if (componentType != ComponentType::Window
        && componentType != ComponentType::Sizer)
        item = nullptr;

    // Fire selection event in plugin for all parents
    if (!m_stopSelectedEvent) {
        PObjectBase parent = obj->GetParent();
        while (parent) {
            IComponent* parentComp = parent->GetObjectInfo()->GetComponent();
            if (parentComp) {
                ObjectBaseMap::iterator parentIt = m_baseobjects.find(parent.get());
                if (parentIt != m_baseobjects.end()) {
                    if (parent->GetObjectInfo()->GetTypeName()
                        == "wizardpagesimple") {
                        WizardPageSimple* wizpage = wxDynamicCast(parentIt->second, WizardPageSimple);
                        SetupWizard(parent, wizpage);
                    }
                    parentComp->OnSelected(parentIt->second);
                }
            }
            parent = parent->GetParent();
        }
    }
    // Look for the active panel - this is where the boxes will be drawn during OnPaint
    // This is the closest parent of type ComponentType::Window
    PObjectBase nextParent = obj->GetParent();
    while (nextParent) {
        IComponent* parentComp = nextParent->GetObjectInfo()->GetComponent();
        if (!parentComp) {
            nextParent.reset();
            break;
        }
        if (parentComp->GetComponentType() == ComponentType::Window)
            break;

        it = m_baseobjects.find(nextParent.get());
        if (it != m_baseobjects.end()) {
            if (wxDynamicCast(it->second, wxStaticBoxSizer))
                break;
        }
        nextParent = nextParent->GetParent();
    }
    // Get the panel to draw on
    wxWindow* selPanel = nullptr;
    if (nextParent) {
        it = m_baseobjects.find(nextParent.get());
        if (it == m_baseobjects.end()) {
            selPanel = m_designer->GetFrameContentPanel();
        } else {
            if (auto* sizer = wxDynamicCast(it->second, wxStaticBoxSizer))
                selPanel = sizer->GetStaticBox();
            else
                selPanel = wxDynamicCast(it->second, wxWindow);
        }
    } else {
        selPanel = m_designer->GetFrameContentPanel();
    }
    // Find the first ComponentType::Window or ComponentType::Sizer
    // If it is a sizer, save it
    wxSizer* sizer = nullptr;
    PObjectBase nextObj = obj->GetParent();
    while (nextObj) {
        IComponent* nextComp = nextObj->GetObjectInfo()->GetComponent();
        if (!nextComp)
            break;

        if (nextComp->GetComponentType() == ComponentType::Sizer) {
            it = m_baseobjects.find(nextObj.get());
            if (it != m_baseobjects.end())
                sizer = wxDynamicCast(it->second, wxSizer);

            break;
        } else if (nextComp->GetComponentType() == ComponentType::Window) {
            break;
        }
        nextObj = nextObj->GetParent();
    }
    m_designer->SetSelectedSizer(sizer);
    m_designer->SetSelectedItem(item);
    m_designer->SetSelectedObject(obj);
    m_designer->SetSelectedPanel(selPanel);
    m_designer->Refresh();
}

void VisualEditor::OnObjectCreated(wxWeaverObjectEvent&)
{
    Create();
}

void VisualEditor::OnObjectRemoved(wxWeaverObjectEvent&)
{
    Create();
}

void VisualEditor::OnPropertyModified(wxWeaverPropertyEvent&)
{
    if (!m_stopModifiedEvent) {
        PObjectBase aux = m_designer->GetSelectedObject();
        Create();
        if (aux) {
            wxWeaverObjectEvent objEvent(wxEVT_WVR_OBJECT_SELECTED, aux);
            this->ProcessEvent(objEvent);
        }
        UpdateVirtualSize();
    }
}

void VisualEditor::OnProjectRefresh(wxWeaverEvent&)
{
    Create();
}

void VisualEditor::OnAuiScan(wxTimerEvent&)
{
    if (m_auiMgr)
        ScanPanes(m_designer->GetFrameContentPanel());
}

wxIMPLEMENT_CLASS(DesignerWindow, InnerFrame);

DesignerWindow::DesignerWindow(wxWindow* parent, int id, const wxPoint& pos,
                               const wxSize& size, long style,
                               const wxString& /*name*/)
    : InnerFrame(parent, id, pos, size, style)
    , m_selSizer(nullptr)
    , m_selItem(nullptr)
    , m_actPanel(nullptr)
{
    ShowTitleBar(false);
    SetGrid(10, 10);
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    GetFrameContentPanel()->PushEventHandler(
        new HighlightPaintHandler(GetFrameContentPanel()));

    Bind(wxEVT_PAINT, &DesignerWindow::OnPaint, this);
}

DesignerWindow::~DesignerWindow()
{
    GetFrameContentPanel()->PopEventHandler(true);
}

void DesignerWindow::SetGrid(int x, int y)
{
    m_x = x;
    m_y = y;
}

void DesignerWindow::OnPaint(wxPaintEvent& event)
{
    // This paint event helps draw the selection boxes
    // when they extend beyond the edges of the content panel
    wxPaintDC dc(this);
    if (m_actPanel == GetFrameContentPanel()) {
        wxPoint origin = GetFrameContentPanel()->GetPosition();
        dc.SetDeviceOrigin(origin.x, origin.y);
        HighlightSelection(dc);
    }
    event.Skip();
}

void DesignerWindow::DrawRectangle(wxDC& dc, const wxPoint& point, const wxSize& size, PObjectBase object)
{
    bool isSizer = (object->GetObjectInfo()->IsSubclassOf("sizer"))
        || object->GetObjectInfo()->IsSubclassOf("gbsizer");
    int min = (isSizer ? 0 : 1);

    int border = object->GetParent()->GetPropertyAsInteger("border");
    if (!border)
        border = min;

    int flag = object->GetParent()->GetPropertyAsInteger("flag");
    int topBorder = !(flag & wxTOP) ? min : border;
    int bottomBorder = !(flag & wxBOTTOM) ? min : border;
    int rightBorder = !(flag & wxRIGHT) ? min : border;
    int leftBorder = !(flag & wxLEFT) ? min : border;

    dc.DrawRectangle(point.x - leftBorder,
                     point.y - topBorder,
                     size.x + leftBorder + rightBorder,
                     size.y + topBorder + bottomBorder);
}

void DesignerWindow::HighlightSelection(wxDC& dc)
{
    // do not highlight if AUI is used in floating mode
    VisualEditor* editor = wxDynamicCast(GetParent(), VisualEditor);
    if (editor && editor->m_auiMgr) {
        wxWindow* windowItem = wxDynamicCast(m_selItem, wxWindow);
        while (windowItem) {
            wxAuiPaneInfo info = editor->m_auiMgr->GetPane(windowItem);
            if (info.IsOk()) {
                if (info.IsFloating())
                    return;
                else
                    break;
            }
            windowItem = windowItem->GetParent();
        }
    }
    wxSize size;
    PObjectBase object = m_selObj.lock();
    if (m_selSizer) {
        wxScrolledWindow* scrolwin
            = wxDynamicCast(m_selSizer->GetContainingWindow(), wxScrolledWindow);
        if (scrolwin)
            scrolwin->FitInside();

        wxPoint point = m_selSizer->GetPosition();
        if (auto* sbSizer = wxDynamicCast(m_selSizer, wxStaticBoxSizer)) {
            /*
                In case of wxStaticBoxSizer, m_actPanel is not a parent window
                of the sizer (m_actPanel==sbSizer->GetStaticBox()).
                Thus we need to convert the sizer's position into coordinates
                of m_actPanel.
                We could do this via parent window of the sizer, but it's hard to
                obtain this window. The m_selSizer->GetContainingWindow() doesn't
                always return the parent window of the sizer, namely in the case
                the m_selSizer is inside another wxStaticBoxSizer
                (at least in MSW build, wxWidgets 3.0.1).
                We convert its StaticBox origin (StaticBox is a window) since
                origins of wxStaticBoxSizer and its StaticBox are the same point.
            */
            point = m_actPanel->ScreenToClient(sbSizer->GetStaticBox()->GetScreenPosition());
        }
        size = m_selSizer->GetSize();

        wxPen bluePen(*wxBLUE, 1, wxPENSTYLE_SOLID);
        dc.SetPen(bluePen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);

        PObjectBase sizerParent = object->FindNearAncestorByBaseClass("sizer");
        if (!sizerParent)
            sizerParent = object->FindNearAncestorByBaseClass("gbsizer");

        if (sizerParent && sizerParent->GetParent())
            DrawRectangle(dc, point, size, sizerParent);
    }

    if (m_selItem) {
        wxPoint point;
        bool shown = false;
        wxWindow* windowItem = wxDynamicCast(m_selItem, wxWindow);
        wxSizer* sizerItem = wxDynamicCast(m_selItem, wxSizer);
        if (windowItem) {
            /*
                In case the windowItem is inside a wxStaticBoxSizer its position
                is relative to the wxStaticBox which is NOT m_actPanel in
                on which the highlight is painted, so get the screen coordinates
                of the item and convert them into client coordinates of the panel
                to get the correct relative coordinates. This doesn't do any harm
                if the item is not inside a wxStaticBoxSizer, if this conversion
                results in a big performance penalty maybe check if the parent
                is a wxStaticBox and only then do this conversion.
            */
            point = m_actPanel->ScreenToClient(windowItem->GetScreenPosition());
            size = windowItem->GetSize();
            shown = windowItem->IsShown();
        } else if (sizerItem) {
            point = sizerItem->GetPosition();
            size = sizerItem->GetSize();
            shown = true;
        } else {
            return;
        }
        if (shown) {
            wxPen redPen(*wxRED, 1, wxPENSTYLE_SOLID);
            dc.SetPen(redPen);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            DrawRectangle(dc, point, size, object);
        }
    }
}

wxMenu* DesignerWindow::GetMenuFromObject(PObjectBase menu)
{
    int lastMenuId = wxID_HIGHEST + 1;
    wxMenu* menuWidget = new wxMenu();
    for (size_t j = 0; j < menu->GetChildCount(); j++) {
        PObjectBase menuItem = menu->GetChild(j);
        if (menuItem->GetTypeName() == "submenu") {
            wxMenuItem* item = new wxMenuItem(menuWidget, lastMenuId++,
                                              menuItem->GetPropertyAsString("label"),
                                              menuItem->GetPropertyAsString("help"),
                                              wxITEM_NORMAL,
                                              GetMenuFromObject(menuItem));
            item->SetBitmap(menuItem->GetPropertyAsBitmap("bitmap"));
            menuWidget->Append(item);
        } else if (menuItem->GetClassName() == "separator") {
            menuWidget->AppendSeparator();
        } else {
            wxString label = menuItem->GetPropertyAsString("label");
            wxString shortcut = menuItem->GetPropertyAsString("shortcut");
            if (!shortcut.IsEmpty())
                label = label + wxChar('\t') + shortcut;

            wxMenuItem* item = new wxMenuItem(menuWidget, lastMenuId++, label,
                                              menuItem->GetPropertyAsString("help"),
                                              (wxItemKind)menuItem->GetPropertyAsInteger("kind"));

            if (!menuItem->GetProperty("bitmap")->IsNull()) {
                wxBitmap unchecked = wxNullBitmap;
                if (!menuItem->GetProperty("unchecked_bitmap")->IsNull())
                    unchecked = menuItem->GetPropertyAsBitmap("unchecked_bitmap");
#ifdef __WXMSW__
                item->SetBitmaps(menuItem->GetPropertyAsBitmap("bitmap"), unchecked);
#elif defined(__WXGTK__)
                item->SetBitmap(menuItem->GetPropertyAsBitmap("bitmap"));
#endif
            } else {
                if (!menuItem->GetProperty("unchecked_bitmap")->IsNull()) {
#ifdef __WXMSW__
                    item->SetBitmaps(wxNullBitmap, menuItem->GetPropertyAsBitmap("unchecked_bitmap"));
#endif
                }
            }
            menuWidget->Append(item);

            if (item->GetKind() == wxITEM_CHECK && menuItem->GetPropertyAsInteger("checked"))
                item->Check(true);

            item->Enable((menuItem->GetPropertyAsInteger("enabled")));
        }
    }
    return menuWidget;
}

void DesignerWindow::SetFrameWidgets(PObjectBase menubar, wxWindow* toolbar,
                                     wxWindow* statusbar, wxWindow* auipanel)
{
    wxWindow* contentPanel = GetFrameContentPanel();
    Menubar* mbWidget = nullptr;

    if (menubar) {
        mbWidget = new Menubar(contentPanel, wxID_ANY);
        for (size_t i = 0; i < menubar->GetChildCount(); i++) {
            PObjectBase menu = menubar->GetChild(i);
            wxMenu* menuWidget = GetMenuFromObject(menu);
            mbWidget->AppendMenu(menu->GetPropertyAsString("label"), menuWidget);
        }
    }
    wxSizer* mainSizer = contentPanel->GetSizer();
    contentPanel->SetSizer(nullptr, false);
    wxSizer* dummySizer = new wxBoxSizer(wxVERTICAL);

    if (mbWidget) {
        dummySizer->Add(mbWidget, 0, wxEXPAND | wxTOP | wxBOTTOM, 0);
        dummySizer->Add(new wxStaticLine(contentPanel, wxID_ANY), 0, wxEXPAND | wxALL, 0);
    }
    wxSizer* contentSizer = dummySizer;
    if (toolbar) {
        if ((toolbar->GetWindowStyle() & wxTB_VERTICAL)) {
            wxSizer* horiz = new wxBoxSizer(wxHORIZONTAL);
            wxSizer* vert = new wxBoxSizer(wxVERTICAL);

            horiz->Add(toolbar, 0, wxEXPAND | wxALL, 0);
            horiz->Add(vert, 1, wxEXPAND, 0);
            dummySizer->Add(horiz, 1, wxEXPAND, 0);
            contentSizer = vert;
        } else {
            dummySizer->Add(toolbar, 0, wxEXPAND | wxALL, 0);
        }
    }
    if (auipanel) {
        contentSizer->Add(auipanel, 1, wxEXPAND | wxALL, 0);
    } else if (mainSizer) {
        contentSizer->Add(mainSizer, 1, wxEXPAND | wxALL, 0);
        if (mainSizer->GetChildren().IsEmpty())
            mainSizer->AddStretchSpacer(1); // Sizers do not expand if they are empty
    } else {
        contentSizer->AddStretchSpacer(1);
    }
    if (statusbar) {
        if (auipanel)
            statusbar->Reparent(contentPanel);

        contentSizer->Add(statusbar, 0, wxEXPAND | wxALL, 0);
    }
    contentPanel->SetSizer(dummySizer, false);
    contentPanel->Layout();
}

DesignerWindow::HighlightPaintHandler::HighlightPaintHandler(wxWindow* win)
    : m_window(win)
{
    Bind(wxEVT_PAINT, &DesignerWindow::HighlightPaintHandler::OnPaint, this);
}

void DesignerWindow::HighlightPaintHandler::OnPaint(wxPaintEvent& event)
{
    wxWindow* aux = m_window;
    while (!aux->IsKindOf(wxCLASSINFO(DesignerWindow)))
        aux = aux->GetParent();

    DesignerWindow* dsgnWin = (DesignerWindow*)aux; // TODO: replace cast type
    if (dsgnWin->GetActivePanel() == m_window) {
        wxPaintDC dc(m_window);
        dsgnWin->HighlightSelection(dc);
    }
    event.Skip();
}
