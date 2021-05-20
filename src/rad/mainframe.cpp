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
#include "rad/mainframe.h"

#include "model/xrcfilter.h"
#include "utils/stringutils.h"
#include "utils/exception.h"
#include "rad/about.h"
#include "rad/appdata.h"
#include "rad/auidockart.h"
#include "rad/auitabart.h"
#include "rad/bitmaps.h"
#include "rad/cpppanel/cpppanel.h"
#include "rad/designer/visualeditor.h"
#include "rad/geninheritclass/geninhertclass.h"
#include "rad/inspector/objinspect.h"
#include "rad/luapanel/luapanel.h"
#include "rad/objecttree/objecttree.h"
#include "rad/palette.h"
#include "rad/phppanel/phppanel.h"
#include "rad/pythonpanel/pythonpanel.h"
#include "rad/title.h"
#include "rad/event.h"
#include "rad/manager.h"
#include "rad/xrcpanel/xrcpanel.h"
#include "rad/debugwindow.h"

#include <wx/config.h>
#include <wx/panel.h>

namespace wxw {
enum {
    ID_ALIGN_LEFT = wxID_HIGHEST + 1,
    ID_ALIGN_CENTER_H,
    ID_ALIGN_RIGHT,
    ID_ALIGN_TOP,
    ID_ALIGN_CENTER_V,
    ID_ALIGN_BOTTOM,
    ID_BORDER_LEFT,
    ID_BORDER_RIGHT,
    ID_BORDER_TOP,
    ID_BORDER_BOTTOM,
    ID_CLIPBOARD_COPY,
    ID_CLIPBOARD_PASTE,
    ID_EXPAND,
    ID_STRETCH,
    ID_IMPORT_XRC,
    ID_PREVIEW_XRC,
    ID_GEN_INHERIT_CLS,
    ID_WINDOW_SWAP, //added by tyysoft to define the swap button ID.
};
enum {
    STATUS_FIELD_PATH = 1,
    STATUS_FIELD_OBJECT = 2
};
} // namespace wxw

using namespace wxw;

// Used to kill focus from propgrid when toolbar or menu items are clicked
// This forces the propgrid to save the cell being edited
class FocusKillerEvtHandler : public wxEvtHandler {
public:
    FocusKillerEvtHandler()
    {
        Bind(wxEVT_MENU, &FocusKillerEvtHandler::OnMenuEvent, this);
    }

    void OnMenuEvent(wxCommandEvent& event)
    {
        wxWindow* windowWithFocus = wxWindow::FindFocus(); // Get window with focus

        // Only send the event if the focus is on the propgrid
        while (windowWithFocus) {
            wxPropertyGrid* propgrid = wxDynamicCast(windowWithFocus, wxPropertyGrid);
            if (propgrid) {
                wxFocusEvent focusEvent(wxEVT_KILL_FOCUS);
                propgrid->GetEventHandler()->ProcessEvent(focusEvent);
                break;
            }
            windowWithFocus = windowWithFocus->GetParent();
        }
        // Add the event to the main window's original handler
        // Add as pending so propgrid saves the property before the event is processed
        GetNextHandler()->AddPendingEvent(event);
    }
};

MainFrame::MainFrame(wxWindow* parent, int id, int style, wxPoint pos, wxSize size)
    : wxFrame(parent, id, wxEmptyString, pos, size, wxDEFAULT_FRAME_STYLE)
    , m_python(nullptr)
    , m_cpp(nullptr)
    , m_lua(nullptr)
    , m_php(nullptr)
    , m_xrc(nullptr)
    , m_panel(nullptr)
    , m_focusKillEvtHandler(new FocusKillerEvtHandler)
    , m_findData(wxFR_DOWN)
    , m_findDialog(nullptr)
    /*
        TODO: autosash function is temporarily disabled due to possible bug(?)
        in wxMSW event system (workaround is needed)
    */
    , m_leftSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_autoSash(false)
    , m_pageSelection(0)
    , m_rightSplitterSashPos(300)
    , m_leftSplitterWidth(300)
    , m_rightSplitterWidth(-300)
    , m_style(style)
{
    // Setup frame icons, title bar, status bar, menubar and toolbar
    wxIconBundle bundle;
    wxIcon ico16, ico32;
    ico16.CopyFromBitmap(AppBitmaps::GetBitmap("app16", 16));
    ico32.CopyFromBitmap(AppBitmaps::GetBitmap("app32", 32));
    bundle.AddIcon(ico16);
    bundle.AddIcon(ico32);
    SetIcons(bundle);
    SetTitle(wxTheApp->GetAppDisplayName());
#if 0
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
#endif
    SetMenuBar(CreateWeaverMenuBar());
    CreateStatusBar(3);
    SetStatusBarPane(0);
    int widths[3] = { -1, -1, 300 };
    SetStatusWidths(sizeof(widths) / sizeof(int), widths);
    CreateWeaverToolBar();

    switch (style) {
    case wxWEAVER_GUI_CLASSIC:
        /*
            +------++-----------------------+
            | Obj. ||  Palette              |
            | Tree ++-----------------------+
            |      ||  Editor               |
            |______||                       |
            |------||                       |
            | Obj. ||                       |
            | Insp.||                       |
            |      ||                       |
            |      ||                       |
            +------++-----------------------+
        */
        CreateClassicGui();
        break;

    case wxWEAVER_GUI_WIDE:
        /*
            +------++-----------------------+
            | Obj. ||  Palette              |
            | Tree ++-------------++--------+
            |      ||  Editor     || Obj.   |
            |      ||             || Insp.  |
            |      ||             ||        |
            |      ||             ||        |
            |      ||             ||        |
            |      ||             ||        |
            |      ||             ||        |
            +------++-------------++--------+
        */
        CreateWideGui();
        break;

    case wxWEAVER_GUI_DOCKABLE:
    default:
        CreateDockableGui();
    }
    LoadSettings();
    Layout();

    AppData()->AddHandler(this->GetEventHandler());
    wxTheApp->SetTopWindow(this);

    PushEventHandler(m_focusKillEvtHandler);

    // So splitter windows can be restored correctly
    if (m_style != wxWEAVER_GUI_DOCKABLE)
        Bind(wxEVT_IDLE, &MainFrame::OnIdle, this);

    m_notebook->Bind(
        wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED,
        &MainFrame::OnAuiNotebookPageChanged, this);
    //==========================================================================
    // Events
    //==========================================================================
    Bind(wxEVT_MENU, &MainFrame::OnFindDialog, this, wxID_FIND);
    Bind(wxEVT_FIND, &MainFrame::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &MainFrame::OnFind, this);
    Bind(wxEVT_FIND_CLOSE, &MainFrame::OnFindClose, this);
    Bind(wxEVT_UPDATE_UI, &MainFrame::OnClipboardPasteUpdateUI, this, ID_CLIPBOARD_PASTE);
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnPreferences, this, wxID_PREFERENCES);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnExit, this, wxID_EXIT);
    //==========================================================================
    // Project
    //==========================================================================
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnOpenRecent, this, wxID_FILE1, wxID_FILE4);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnNewProject, this, wxID_NEW);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnOpenProject, this, wxID_OPEN);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnSaveProject, this, wxID_SAVE);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnSaveAsProject, this, wxID_SAVEAS);
    //==========================================================================
    // Undo/Redo
    //==========================================================================
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnUndo, this, wxID_UNDO);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnRedo, this, wxID_REDO);
    //==========================================================================
    // Clipboard
    //==========================================================================
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnCut, this, wxID_CUT);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnCopy, this, wxID_COPY);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnPaste, this, wxID_PASTE);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnDelete, this, wxID_DELETE);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnClipboardCopy, this, ID_CLIPBOARD_COPY);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnClipboardPaste, this, ID_CLIPBOARD_PASTE);
    //==========================================================================
    // Move operations
    //==========================================================================
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnMoveUp, this, wxID_UP);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnMoveDown, this, wxID_DOWN);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnMoveLeft, this, wxID_BACKWARD);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnMoveRight, this, wxID_FORWARD);
    //==========================================================================
    // CodeGenerator
    //==========================================================================
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnGenerateCode, this, wxID_EXECUTE);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnGenInhertedClass, this, ID_GEN_INHERIT_CLS);
    //==========================================================================
    // Widget's alignment
    //==========================================================================
#if 1
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeAlignment, this,
         ID_ALIGN_LEFT, ID_ALIGN_BOTTOM);
#else
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeAlignment, this, ID_ALIGN_LEFT);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeAlignment, this, ID_ALIGN_CENTER_H);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeAlignment, this, ID_ALIGN_RIGHT);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeAlignment, this, ID_ALIGN_TOP);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeAlignment, this, ID_ALIGN_CENTER_V);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeAlignment, this, ID_ALIGN_BOTTOM);
#endif
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnToggleExpand, this, ID_EXPAND);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnToggleStretch, this, ID_STRETCH);
    //==========================================================================
    // Widget's borders
    //==========================================================================
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnChangeBorder, this,
         ID_BORDER_LEFT, ID_BORDER_BOTTOM);

    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnImportXrc, this, ID_IMPORT_XRC);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnXrcPreview, this, ID_PREVIEW_XRC);
    Bind(wxEVT_COMMAND_TOOL_CLICKED, &MainFrame::OnWindowSwap, this, ID_WINDOW_SWAP);

    Bind(wxEVT_WVR_CODE_GENERATION, &MainFrame::OnCodeGeneration, this);
    Bind(wxEVT_WVR_OBJECT_CREATED, &MainFrame::OnObjectCreated, this);
    Bind(wxEVT_WVR_OBJECT_REMOVED, &MainFrame::OnObjectRemoved, this);
    Bind(wxEVT_WVR_OBJECT_EXPANDED, &MainFrame::OnObjectExpanded, this);
    Bind(wxEVT_WVR_OBJECT_SELECTED, &MainFrame::OnObjectSelected, this);
    Bind(wxEVT_WVR_PROJECT_LOADED, &MainFrame::OnProjectLoaded, this);
    Bind(wxEVT_WVR_PROJECT_REFRESH, &MainFrame::OnProjectRefresh, this);
    Bind(wxEVT_WVR_PROJECT_SAVED, &MainFrame::OnProjectSaved, this);
    Bind(wxEVT_WVR_PROPERTY_MODIFIED, &MainFrame::OnPropertyModified, this);
    Bind(wxEVT_WVR_EVENT_HANDLER_MODIFIED, &MainFrame::OnEventHandlerModified, this);
}

MainFrame::~MainFrame()
{
    m_notebook->Unbind(
        wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED,
        &MainFrame::OnAuiNotebookPageChanged, this);

#ifdef __WXMAC__
    // work around problem on wxMac
    if (m_style != wxWEAVER_GUI_DOCKABLE) {
        m_rightSplitter->GetWindow1()->GetSizer()->Detach(m_notebook);
        m_notebook->Destroy();
    }
#endif
    if (m_style == wxWEAVER_GUI_DOCKABLE)
        m_mgr.UnInit();

    // the focus killer event handler
    RemoveEventHandler(m_focusKillEvtHandler);
    delete m_focusKillEvtHandler;

    AppData()->RemoveHandler(this->GetEventHandler());
    delete m_findDialog;
}

void MainFrame::LoadSettings(const wxString& name)
{
    wxConfigBase* config = wxConfigBase::Get();
    config->SetPath(name);

    wxString perspective;
    wxSize bestSize = GetBestSize();
    bool maximized, iconized;
    int x, y, w, h;

    // disabled in default due to possible bug(?) in wxMSW
    config->Read("AutoSash", &m_autoSash, false);
#if 0
    config->Read("LeftSplitterWidth", &m_leftSplitterWidth, 300);
    config->Read("RightSplitterWidth", &m_rightSplitterWidth, -300);
    config->Read("RightSplitterType", &m_rightSplitterType, "editor");
#endif
    config->Read("CurrentDirectory", &m_currentDir, "./projects");
    config->Read("RecentFile0", &m_recentProjects[0]);
    config->Read("RecentFile1", &m_recentProjects[1]);
    config->Read("RecentFile2", &m_recentProjects[2]);
    config->Read("RecentFile3", &m_recentProjects[3]);
    config->Read("Style", &m_style, wxWEAVER_GUI_DOCKABLE);
    config->Read("Perspective", &perspective);
    config->Read("IsMaximized", &maximized, false);
    config->Read("IsIconized", &iconized, false);
    config->Read("Left", &x, 0);
    config->Read("Top", &y, 0);
    config->Read("Width", &w, bestSize.GetWidth());
    config->Read("Height", &h, bestSize.GetHeight());

    SetSize(x, y, w, h);

    if (maximized) {
        Maximize(maximized);
    } else if (iconized) {
        Iconize(iconized);
    }
    if (m_style == wxWEAVER_GUI_DOCKABLE) {
        if (!perspective.empty()) {
            m_mgr.LoadPerspective(perspective);
            m_mgr.Update();
        }
    }
    UpdateRecentProjects();
}

void MainFrame::SaveSettings(const wxString& name)
{
    m_objInsp->SaveSettings();
    m_palette->SaveSettings();

    wxConfigBase* config = wxConfigBase::Get();
    config->SetPath(name);

    bool isIconized = IsIconized();
    bool isMaximized = IsMaximized();

    if (m_style == wxWEAVER_GUI_DOCKABLE) {
        wxString perspective = m_mgr.SavePerspective();
        config->Write("Perspective", perspective);
    }
    if (!isMaximized) {
        config->Write("Left", isIconized ? -1 : GetPosition().x);
        config->Write("Top", isIconized ? -1 : GetPosition().y);
        config->Write("Width", isIconized ? -1 : GetSize().GetWidth());
        config->Write("Height", isIconized ? -1 : GetSize().GetHeight());
    }
    config->Write("IsMaximized", isMaximized);
    config->Write("IsIconized", isIconized);
    config->Write("CurrentDirectory", m_currentDir);
    config->Write("RecentFile0", m_recentProjects[0]);
    config->Write("RecentFile1", m_recentProjects[1]);
    config->Write("RecentFile2", m_recentProjects[2]);
    config->Write("RecentFile3", m_recentProjects[3]);
    config->Write("Style", m_style);

    if (m_leftSplitter) {
        int leftSashWidth = m_leftSplitter->GetSashPosition();
        config->Write("LeftSplitterWidth", leftSashWidth);
    }
    if (m_rightSplitter) {
        switch (m_style) {
        case wxWEAVER_GUI_WIDE: {
            int rightSash = -1
                * (m_rightSplitter->GetSize().GetWidth()
                   - m_rightSplitter->GetSashPosition());
            config->Write("RightSplitterWidth", rightSash);

            if (m_rightSplitter->GetWindow1()->GetChildren()[0]->GetChildren()[0]->GetLabel() == _("Editor")) {
                config->Write("RightSplitterType", "editor");
            } else {
                config->Write("RightSplitterType", "prop");
            }
            break;
        }
        case wxWEAVER_GUI_CLASSIC: {
            int rightSash = -1 * (m_rightSplitter->GetSize().GetHeight() - m_rightSplitter->GetSashPosition());
            config->Write("RightSplitterWidth", rightSash);
            break;
        }
        default:
            break;
        }
    }
}

void MainFrame::OnPreferences(wxCommandEvent&)
{
    wxDialog* dlg = AppData()->GetSettingsDialog(this);
    if (dlg) {
        dlg->ShowModal();
        dlg->Destroy();
    }
}

void MainFrame::OnSaveProject(wxCommandEvent& event)

{
    wxString filename = AppData()->GetProjectFileName();
    if (filename == "")
        OnSaveAsProject(event);
    else {
        try {
            AppData()->SaveProject(filename);
            InsertRecentProject(filename);
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
        }
    }
}

void MainFrame::OnSaveAsProject(wxCommandEvent&)
{
    wxFileDialog* dialog
        = new wxFileDialog(
            this, _("Save Project"), m_currentDir, "",
            _("wxWeaver Project File")
                + " (*.fbp)|*.fbp|"
                + _("All files")
                + " (*.*)|*.*",
            wxFD_SAVE);

    if (dialog->ShowModal() == wxID_OK) {
        m_currentDir = dialog->GetDirectory();
        wxString filename = dialog->GetPath();

        // Add the default extension if none was chosen
        wxFileName file(filename);
        if (!file.HasExt()) {
            file.SetExt("fbp");
            filename = file.GetFullPath();
        }
        // Check the file whether exists or not
        if (file.FileExists() == true) {
            wxMessageDialog msg_box(
                this,
                _("The file already exists. Do you want to replace it?"),
                _("Overwrite the file"),
                wxYES_NO | wxICON_INFORMATION | wxNO_DEFAULT);

            if (msg_box.ShowModal() == wxID_NO) {
                dialog->Destroy();
                return;
            }
        }
        try {
            AppData()->SaveProject(filename);
            InsertRecentProject(filename);
        } catch (wxWeaverException& ex) {
            wxLogError(ex.what());
        }
    }
    dialog->Destroy();
}

void MainFrame::OnOpenProject(wxCommandEvent&)
{
    if (!SaveWarning())
        return;

    wxFileDialog* dialog
        = new wxFileDialog(
            this, _("Open Project"), m_currentDir, "",
            _("wxWeaver Project File")
                + " (*.fbp)|*.fbp|"
                + _("All files")
                + " (*.*)|*.*",
            wxFD_OPEN);

    if (dialog->ShowModal() == wxID_OK) {
        m_currentDir = dialog->GetDirectory();
        wxString filename = dialog->GetPath();

        if (AppData()->LoadProject(filename))
            InsertRecentProject(filename);
    };
    dialog->Destroy();
}

void MainFrame::OnOpenRecent(wxCommandEvent& event)
{
    if (!SaveWarning())
        return;

    int i = event.GetId() - wxID_FILE1;
    assert(i >= 0 && i < 4);
    wxFileName filename(m_recentProjects[i]);

    if (filename.FileExists()) {
        if (AppData()->LoadProject(filename.GetFullPath())) {
            m_currentDir = filename.GetPath();
            InsertRecentProject(filename.GetFullPath());
        }
    } else {
        if (wxMessageBox(
                wxString::Format(
                    _("The project file '%s' doesn't exist. Would you like to remove it from the recent files list?"),
                    filename.GetName().GetData()),
                _("Open recent project"), wxICON_WARNING | wxYES_NO)
            == wxYES) {
            m_recentProjects[i] = "";
            UpdateRecentProjects();
        }
    }
}

void MainFrame::OnImportXrc(wxCommandEvent&)
{
    wxFileDialog* dialog
        = new wxFileDialog(this, _("Import XRC file"), m_currentDir,
                           "example.xrc", "*.xrc", wxFD_OPEN);

    if (dialog->ShowModal() == wxID_OK) {
        m_currentDir = dialog->GetDirectory();
        try {
            ticpp::Document doc;
            XMLUtils::LoadXMLFile(doc, false, dialog->GetPath());

            XrcLoader xrc;
            xrc.SetObjectDatabase(AppData()->GetObjectDatabase());

            PObjectBase project = xrc.GetProject(&doc);
            if (project)
                AppData()->MergeProject(project);
            else
                wxLogError("Error while loading XRC");

        } catch (wxWeaverException& ex) {
            wxLogError("Error Loading XRC: %s", ex.what());
        }
    }
    dialog->Destroy();
}

void MainFrame::OnNewProject(wxCommandEvent&)
{
    if (!SaveWarning())
        return;

    AppData()->NewProject();
}

void MainFrame::OnGenerateCode(wxCommandEvent&)
{
    AppData()->GenerateCode(false);
}

void MainFrame::OnAbout(wxCommandEvent&)
{
    AboutDialog dlg(this);
    dlg.ShowModal();
    dlg.Destroy();
}

void MainFrame::OnExit(wxCommandEvent&)
{
    Close();
}

void MainFrame::OnClose(wxCloseEvent& event)
{
    if (!SaveWarning())
        return;

    SaveSettings();

    if (m_style != wxWEAVER_GUI_DOCKABLE) {
        m_rightSplitter->Unbind(
            wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED,
            &MainFrame::OnSplitterChanged, this);
    }
    event.Skip();
}

void MainFrame::OnProjectLoaded(wxWeaverEvent&)
{
    GetStatusBar()->SetStatusText(_("Project Loaded!"));
    PObjectBase project = AppData()->GetProjectData();
    if (project) {
        wxString objDetails
            = wxString::Format(
                _("Name: %s | Class: %s"),
                project->GetPropertyAsString("name").c_str(),
                project->GetClassName().c_str());
        GetStatusBar()->SetStatusText(objDetails, STATUS_FIELD_OBJECT);
    }
    UpdateFrame();
}

void MainFrame::OnProjectSaved(wxWeaverEvent&)
{
    GetStatusBar()->SetStatusText(_("Project Saved!"));
    UpdateFrame();
}

void MainFrame::OnObjectExpanded(wxWeaverObjectEvent&)
{
    UpdateFrame();
}

void MainFrame::OnObjectSelected(wxWeaverObjectEvent& event)
{
    PObjectBase obj = event.GetWvrObject();

    LogDebug("MainFrame::OnObjectSelected");

    // resize sash position if necessary
    if (m_autoSash) {
        if (m_style == wxWEAVER_GUI_WIDE) {
            switch (m_pageSelection) {
            case 1: // CPP panel
                break;

            case 2: // Python panel
                break;

            case 3: // PHP panel
                break;

            case 4: // LUA panel
                break;

            case 5: // XRC panel
                break;

            default:
                if (m_visualEdit) {
                    // If selected object is not a Frame or a Panel or a dialog,
                    // we won't adjust the sash position
                    if (obj->GetObjectTypeName() == "form"
                        || obj->GetObjectTypeName() == "wizard"
                        || obj->GetObjectTypeName() == "menubar_form"
                        || obj->GetObjectTypeName() == "toolbar_form") {
                        int sashPos = m_rightSplitter->GetSashPosition();
                        wxSize panelSize = m_visualEdit->GetVirtualSize();
                        LogDebug(
                            "MainFrame::OnObjectSelected > sash pos = %d", sashPos);
                        LogDebug(
                            "MainFrame::OnObjectSelected > virtual width = %d",
                            panelSize.GetWidth());

                        if (panelSize.GetWidth() > sashPos) {
                            //set the sash position
                            LogDebug("MainFrame::OnObjectSelected > set sash position");
                            m_rightSplitterSashPos = panelSize.GetWidth();
                            m_rightSplitter->SetSashPosition(m_rightSplitterSashPos);
                        }
                    }
                }
                break;
            }
        }
    }
    wxString name;
    PProperty prop(obj->GetProperty("name"));
    if (prop)
        name = prop->GetValueAsString();
    else
        name = "\"Unknown\"";
#if 0
    GetStatusBar()->SetStatusText(_("Object ") + name + _(" Selected!"));
#endif
    wxString objDetails = wxString::Format(
        _("Name: %s | Class: %s"), name.c_str(), obj->GetClassName().c_str());

    GetStatusBar()->SetStatusText(objDetails, STATUS_FIELD_OBJECT);
    UpdateFrame();
}

void MainFrame::OnObjectCreated(wxWeaverObjectEvent& event)
{
    wxString message;
    LogDebug("MainFrame::OnObjectCreated");

    if (event.GetWvrObject()) {
        message.Printf(_("Object '%s' of class '%s' created."),
                       event.GetWvrObject()->GetPropertyAsString("name").c_str(),
                       event.GetWvrObject()->GetClassName().c_str());
    } else {
        message = "Impossible to create the object."
                  "Did you forget to add a sizer/parent object or turn on/off an AUI management?";

        wxMessageBox(message, wxTheApp->GetAppDisplayName(), wxICON_WARNING | wxOK);
    }
    GetStatusBar()->SetStatusText(message);
    UpdateFrame();
}

void MainFrame::OnObjectRemoved(wxWeaverObjectEvent& event)
{
    wxString message;
    message.Printf(_("Object '%s' removed."),
                   event.GetWvrObject()->GetPropertyAsString("name").c_str());
    GetStatusBar()->SetStatusText(message);
    UpdateFrame();
}

void MainFrame::OnPropertyModified(wxWeaverPropertyEvent& event)
{
    PProperty prop = event.GetWvrProperty();
    if (prop) {
        if (prop->GetObject() == AppData()->GetSelectedObject()) {
            if (!prop->GetName().CmpNoCase("name")) {
                wxString oldDetails = GetStatusBar()->GetStatusText(STATUS_FIELD_OBJECT);
                wxString newDetails;
                size_t pipeIdx = oldDetails.find("|");
                if (pipeIdx != oldDetails.npos) {
                    newDetails.Printf(
                        "Name: %s %s", prop->GetValueAsString().c_str(),
                        oldDetails.substr(pipeIdx).c_str());

                    GetStatusBar()->SetStatusText(newDetails, STATUS_FIELD_OBJECT);
                }
            }
            GetStatusBar()->SetStatusText(_("Property Modified!"));
        }
        /*
            When you change the sizeritem properties, the object modified is not
            the same that the selected object because is a sizeritem object.
            It's necessary to update the frame for the toolbar buttons.
        */
        UpdateFrame();
    }
}

void MainFrame::OnEventHandlerModified(wxWeaverEventHandlerEvent& event)
{
    wxString message;
    message.Printf("_(Event handler '%s' of object '%s' modified."),
        event.GetWvrEventHandler()->GetName().c_str(),
        event.GetWvrEventHandler()->GetObject()->GetPropertyAsString("name").c_str();

    GetStatusBar()->SetStatusText(message);
    UpdateFrame();
}

void MainFrame::OnCodeGeneration(wxWeaverEvent& event)
{
    // Using the previously unused Id field in the event to carry a boolean
    bool panelOnly = event.GetId();
    if (panelOnly)
        GetStatusBar()->SetStatusText("Code Generated!");
}

void MainFrame::OnProjectRefresh(wxWeaverEvent&)
{
    PObjectBase project = AppData()->GetProjectData();
    if (project) {
        wxString objDetails = wxString::Format(
            _("Name: %s | Class: %s"),
            project->GetPropertyAsString("name").c_str(),
            project->GetClassName().c_str());

        GetStatusBar()->SetStatusText(objDetails, STATUS_FIELD_OBJECT);
    }
    UpdateFrame();
}

void MainFrame::OnUndo(wxCommandEvent&)
{
    AppData()->Undo();
}

void MainFrame::OnRedo(wxCommandEvent&)
{
    AppData()->Redo();
}

void MainFrame::UpdateLayoutTools()
{
    int option = -1;
    int border = 0;
    int flag = 0;
    int orient = 0;

    bool gotLayoutSettings = AppData()->GetLayoutSettings(
        AppData()->GetSelectedObject(), &flag, &option, &border, &orient);

    wxToolBar* toolbar = GetToolBar();
    wxMenu* menuEdit = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Edit")));

    // Enable the layout tools if there are layout settings, else disable the tools
    menuEdit->Enable(ID_EXPAND, gotLayoutSettings);
    toolbar->EnableTool(ID_EXPAND, gotLayoutSettings);
    menuEdit->Enable(ID_STRETCH, option >= 0);
    toolbar->EnableTool(ID_STRETCH, option >= 0);

    bool enableHorizontalTools = (orient != wxHORIZONTAL) && gotLayoutSettings;
    menuEdit->Enable(ID_ALIGN_LEFT, enableHorizontalTools);
    toolbar->EnableTool(ID_ALIGN_LEFT, enableHorizontalTools);
    menuEdit->Enable(ID_ALIGN_CENTER_H, enableHorizontalTools);
    toolbar->EnableTool(ID_ALIGN_CENTER_H, enableHorizontalTools);
    menuEdit->Enable(ID_ALIGN_RIGHT, enableHorizontalTools);
    toolbar->EnableTool(ID_ALIGN_RIGHT, enableHorizontalTools);

    bool enableVerticalTools = (orient != wxVERTICAL) && gotLayoutSettings;
    menuEdit->Enable(ID_ALIGN_TOP, enableVerticalTools);
    toolbar->EnableTool(ID_ALIGN_TOP, enableVerticalTools);
    menuEdit->Enable(ID_ALIGN_CENTER_V, enableVerticalTools);
    toolbar->EnableTool(ID_ALIGN_CENTER_V, enableVerticalTools);
    menuEdit->Enable(ID_ALIGN_BOTTOM, enableVerticalTools);
    toolbar->EnableTool(ID_ALIGN_BOTTOM, enableVerticalTools);

    toolbar->EnableTool(ID_BORDER_TOP, gotLayoutSettings);
    toolbar->EnableTool(ID_BORDER_RIGHT, gotLayoutSettings);
    toolbar->EnableTool(ID_BORDER_LEFT, gotLayoutSettings);
    toolbar->EnableTool(ID_BORDER_BOTTOM, gotLayoutSettings);

    // Toggle the toolbar buttons according to the properties, if there are layout settings
    toolbar->ToggleTool(ID_EXPAND, ((flag & wxEXPAND)) && gotLayoutSettings);
    toolbar->ToggleTool(ID_STRETCH, (option > 0) && gotLayoutSettings);
    toolbar->ToggleTool(ID_ALIGN_LEFT, !((flag & (wxALIGN_RIGHT | wxALIGN_CENTER_HORIZONTAL))) && enableHorizontalTools);
    toolbar->ToggleTool(ID_ALIGN_CENTER_H, ((flag & wxALIGN_CENTER_HORIZONTAL)) && enableHorizontalTools);
    toolbar->ToggleTool(ID_ALIGN_RIGHT, ((flag & wxALIGN_RIGHT)) && enableHorizontalTools);
    toolbar->ToggleTool(ID_ALIGN_TOP, !((flag & (wxALIGN_BOTTOM | wxALIGN_CENTER_VERTICAL))) && enableVerticalTools);
    toolbar->ToggleTool(ID_ALIGN_CENTER_V, ((flag & wxALIGN_CENTER_VERTICAL)) && enableVerticalTools);
    toolbar->ToggleTool(ID_ALIGN_BOTTOM, ((flag & wxALIGN_BOTTOM)) && enableVerticalTools);

    toolbar->ToggleTool(ID_BORDER_TOP, ((flag & wxTOP)) && gotLayoutSettings);
    toolbar->ToggleTool(ID_BORDER_RIGHT, ((flag & wxRIGHT)) && gotLayoutSettings);
    toolbar->ToggleTool(ID_BORDER_LEFT, ((flag & wxLEFT)) && gotLayoutSettings);
    toolbar->ToggleTool(ID_BORDER_BOTTOM, ((flag & wxBOTTOM)) && gotLayoutSettings);
}

void MainFrame::UpdateFrame()
{
    // Build the title
    wxString filename = AppData()->GetProjectFileName();
    wxString file;
    if (filename.empty()) {
        file = "untitled";
    } else {
        wxFileName fn(filename);
        file = fn.GetName();
    }
    SetTitle(wxString::Format(
        "%s%s - %s v%s%s",
        AppData()->IsModified() ? "*" : "", file.c_str(),
        wxTheApp->GetAppDisplayName(), VERSION, REVISION));

    GetStatusBar()->SetStatusText(filename, STATUS_FIELD_PATH);

    // Enable/Disable toolbar and menu entries
    wxToolBar* toolbar = GetToolBar();

    wxMenu* menuFile = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("File")));
    menuFile->Enable(wxID_SAVE, AppData()->IsModified());
    toolbar->EnableTool(wxID_SAVE, AppData()->IsModified());

    wxMenu* menuEdit = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Edit")));

    bool redo = AppData()->CanRedo();
    menuEdit->Enable(wxID_REDO, redo);
    toolbar->EnableTool(wxID_REDO, redo);

    bool undo = AppData()->CanUndo();
    menuEdit->Enable(wxID_UNDO, undo);
    toolbar->EnableTool(wxID_UNDO, undo);

    bool copy = AppData()->CanCopyObject();
    bool isEditor = (_("Designer") != m_notebook->GetPageText(m_notebook->GetSelection()));
    menuEdit->Enable(wxID_FIND, isEditor);

    menuEdit->Enable(ID_CLIPBOARD_COPY, copy);

    menuEdit->Enable(wxID_COPY, copy || isEditor);
    toolbar->EnableTool(wxID_COPY, copy || isEditor);

    menuEdit->Enable(wxID_CUT, copy);
    toolbar->EnableTool(wxID_CUT, copy);

    menuEdit->Enable(wxID_DELETE, copy);
    toolbar->EnableTool(wxID_DELETE, copy);

    menuEdit->Enable(wxID_UP, copy);
    menuEdit->Enable(wxID_DOWN, copy);
    menuEdit->Enable(wxID_BACKWARD, copy);
    menuEdit->Enable(wxID_FORWARD, copy);

    bool paste = AppData()->CanPasteObject();
    menuEdit->Enable(wxID_PASTE, paste);
    toolbar->EnableTool(wxID_PASTE, paste);

    menuEdit->Enable(wxID_PASTE, AppData()->CanPasteObjectFromClipboard());
    UpdateLayoutTools();
}

void MainFrame::UpdateRecentProjects()
{
    wxMenu* menuFile = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("File")));

    // borramos los items del menu de los projectos recientes
    for (int i = 0; i < 4; i++) {
        if (menuFile->FindItem(wxID_FILE1 + i))
            menuFile->Destroy(wxID_FILE1 + i);
    }
    wxMenuItem* mruSep = menuFile->FindItemByPosition(menuFile->GetMenuItemCount() - 1);
    if (mruSep->IsSeparator()) {
        menuFile->Destroy(mruSep);
    }
    // remove empty filenames and 'compress' the rest
    int fi = 0;
    for (int i = 0; i < 4; i++) {
        if (!m_recentProjects[i].IsEmpty())
            m_recentProjects[fi++] = m_recentProjects[i];
    }
    for (int i = fi; i < 4; i++)
        m_recentProjects[i] = "";

    if (!m_recentProjects[0].IsEmpty())
        menuFile->AppendSeparator();

    // creamos los nuevos ficheros recientes
    for (size_t i = 0; i < 4 && !m_recentProjects[i].IsEmpty(); i++)
        menuFile->Append(wxID_FILE1 + i, m_recentProjects[i], "");
}

void MainFrame::InsertRecentProject(const wxString& file)
{
    bool found = false;
    int i;
    for (i = 0; i < 4 && !found; i++)
        found = (file == m_recentProjects[i]);

    if (found) // en i-1 está la posición encontrada (0 < i < 4)
    {
        // desplazamos desde 0 hasta i-1 una posición a la derecha
        for (i = i - 1; i > 0; i--)
            m_recentProjects[i] = m_recentProjects[i - 1];
    } else if (!found) {
        for (i = 3; i > 0; i--)
            m_recentProjects[i] = m_recentProjects[i - 1];
    }
    m_recentProjects[0] = file;
    UpdateRecentProjects();
}

void MainFrame::OnCopy(wxCommandEvent&)

{
    wxWindow* focusedWindow = wxWindow::FindFocus();
    if (focusedWindow && focusedWindow->IsKindOf(wxCLASSINFO(wxStyledTextCtrl))) {
        ((wxStyledTextCtrl*)focusedWindow)->Copy();
    } else {
        AppData()->CopyObject(AppData()->GetSelectedObject());
        UpdateFrame();
    }
}

void MainFrame::OnCut(wxCommandEvent&)
{
    wxWindow* focusedWindow = wxWindow::FindFocus();
    if (focusedWindow && focusedWindow->IsKindOf(wxCLASSINFO(wxStyledTextCtrl))) {
        ((wxStyledTextCtrl*)focusedWindow)->Cut();
    } else {
        AppData()->CutObject(AppData()->GetSelectedObject());
        UpdateFrame();
    }
}

void MainFrame::OnDelete(wxCommandEvent&)
{
    AppData()->RemoveObject(AppData()->GetSelectedObject());
    UpdateFrame();
}

void MainFrame::OnPaste(wxCommandEvent&)
{
    wxWindow* focusedWindow = wxWindow::FindFocus();
    if (focusedWindow && focusedWindow->IsKindOf(wxCLASSINFO(wxStyledTextCtrl))) {
        ((wxStyledTextCtrl*)focusedWindow)->Paste();
    } else {
        AppData()->PasteObject(AppData()->GetSelectedObject());
        UpdateFrame();
    }
}

void MainFrame::OnClipboardCopy(wxCommandEvent&)
{
    AppData()->CopyObjectToClipboard(AppData()->GetSelectedObject());
    UpdateFrame();
}

void MainFrame::OnClipboardPaste(wxCommandEvent&)
{
    AppData()->PasteObjectFromClipboard(AppData()->GetSelectedObject());
    UpdateFrame();
}

void MainFrame::OnClipboardPasteUpdateUI(wxUpdateUIEvent& e)
{
    e.Enable(AppData()->CanPasteObjectFromClipboard());
}

void MainFrame::OnToggleExpand(wxCommandEvent&)
{
    AppData()->ToggleExpandLayout(AppData()->GetSelectedObject());
}

void MainFrame::OnToggleStretch(wxCommandEvent&)
{
    AppData()->ToggleStretchLayout(AppData()->GetSelectedObject());
}

void MainFrame::OnMoveUp(wxCommandEvent&)
{
    AppData()->MovePosition(AppData()->GetSelectedObject(), false, 1);
}

void MainFrame::OnMoveDown(wxCommandEvent&)
{
    AppData()->MovePosition(AppData()->GetSelectedObject(), true, 1);
}

void MainFrame::OnMoveLeft(wxCommandEvent&)
{
    AppData()->MoveHierarchy(AppData()->GetSelectedObject(), true);
}

void MainFrame::OnMoveRight(wxCommandEvent&)
{
    AppData()->MoveHierarchy(AppData()->GetSelectedObject(), false);
}

void MainFrame::OnChangeAlignment(wxCommandEvent& event)
{
    int align = 0;
    bool vertical
        = (event.GetId() == ID_ALIGN_TOP
           || event.GetId() == ID_ALIGN_BOTTOM
           || event.GetId() == ID_ALIGN_CENTER_V);

    switch (event.GetId()) {
    case ID_ALIGN_RIGHT:
        align = wxALIGN_RIGHT;
        break;

    case ID_ALIGN_CENTER_H:
        align = wxALIGN_CENTER_HORIZONTAL;
        break;

    case ID_ALIGN_BOTTOM:
        align = wxALIGN_BOTTOM;
        break;

    case ID_ALIGN_CENTER_V:
        align = wxALIGN_CENTER_VERTICAL;
        break;
    }
    AppData()->ChangeAlignment(AppData()->GetSelectedObject(), align, vertical);
    UpdateLayoutTools();
}

void MainFrame::OnChangeBorder(wxCommandEvent& e)
{
    int border;
    switch (e.GetId()) {
    case ID_BORDER_LEFT:
        border = wxLEFT;
        break;

    case ID_BORDER_RIGHT:
        border = wxRIGHT;
        break;

    case ID_BORDER_TOP:
        border = wxTOP;
        break;

    case ID_BORDER_BOTTOM:
        border = wxBOTTOM;
        break;

    default:
        border = 0;
        break;
    }
    AppData()->ToggleBorderFlag(AppData()->GetSelectedObject(), border);
    UpdateLayoutTools();
}

void MainFrame::OnXrcPreview(wxCommandEvent& WXUNUSED(e))
{
    AppData()->ShowXrcPreview();
#if 0
    if (m_style == wxWEAVER_GUI_DOCKABLE) {
        wxAuiPaneInfoArray& allPanes = m_mgr.GetAllPanes();
        for (int i = 0, count = allPanes.GetCount(); i < count; ++i)
            wxAuiPaneInfo info = allPanes.Item(i);
    }
#endif
}

void MainFrame::OnGenInhertedClass(wxCommandEvent& WXUNUSED(e))
{
    wxString filePath;
    try {
        // Get the output path
        filePath = AppData()->GetOutputPath();
    } catch (wxWeaverException& ex) {
        wxLogWarning(ex.what());
        return;
    }
    // Show the dialog
    PObjectBase project = AppData()->GetProjectData();
    if (project->IsNull(_("file"))) {
        wxLogWarning("You must set the \"file\" property of the project before generating inherited classes.");
        return;
    }
    GenInheritedClassDlg dlg(this, project);

    if (wxID_OK != dlg.ShowModal())
        return;

    std::vector<GenClassDetails> selectedForms;
    dlg.GetFormsSelected(&selectedForms);

    for (size_t i = 0; i < selectedForms.size(); ++i) {
        const GenClassDetails& details = selectedForms[i];

        // Create the class and files.
        AppData()->GenerateInheritedClass(
            details.m_form, details.m_className, filePath, details.m_fileName);
    }
    wxMessageBox(wxString::Format(
        _("Class(es) generated to \'%s\'."), filePath.c_str(),
        wxTheApp->GetAppDisplayName()));
}

bool MainFrame::SaveWarning()
{
    int result = wxYES;

    if (AppData()->IsModified()) {
        result = ::wxMessageBox(_("Current project file has been modified...\n"
                                  "Do you want to save the changes?"),
                                _("Save project"),
                                wxYES | wxNO | wxCANCEL,
                                this);
        if (result == wxYES) {
            wxCommandEvent dummy;
            OnSaveProject(dummy);
        }
    }
    return (result != wxCANCEL);
}

void MainFrame::OnAuiNotebookPageChanged(wxAuiNotebookEvent& event)
{
    UpdateFrame();

    if (m_autoSash) {
        m_pageSelection = event.GetSelection();
        LogDebug(
            "MainFrame::OnAuiNotebookPageChanged > selection = %d", m_pageSelection);

        wxSize panelSize;
        int sashPos;
        if (m_style != wxWEAVER_GUI_CLASSIC) {
            switch (m_pageSelection) {
            case 1: // CPP panel
                if (m_cpp && m_rightSplitter) {
                    panelSize = m_cpp->GetClientSize();
                    sashPos = m_rightSplitter->GetSashPosition();

                    LogDebug(
                        "MainFrame::OnAuiNotebookPageChanged > CPP panel: width = %d sash pos = %d",
                        panelSize.GetWidth(), sashPos);

                    if (panelSize.GetWidth() > sashPos) {
                        // set the sash position
                        LogDebug("MainFrame::OnAuiNotebookPageChanged > reset sash position");

                        m_rightSplitter->SetSashPosition(panelSize.GetWidth());
                    }
                }
                break;

            case 2: // Python panel
                if (m_python && m_rightSplitter) {
                    panelSize = m_python->GetClientSize();
                    sashPos = m_rightSplitter->GetSashPosition();

                    LogDebug(
                        "MainFrame::OnAuiNotebookPageChanged > Python panel: width = %d sash pos = %d",
                        panelSize.GetWidth(), sashPos);

                    if (panelSize.GetWidth() > sashPos) {
                        // set the sash position
                        LogDebug("MainFrame::OnAuiNotebookPageChanged > reset sash position");

                        m_rightSplitter->SetSashPosition(panelSize.GetWidth());
                    }
                }
                break;

            case 3: // PHP panel
                if (m_php && m_rightSplitter) {
                    panelSize = m_xrc->GetClientSize();
                    sashPos = m_rightSplitter->GetSashPosition();

                    LogDebug(
                        "MainFrame::OnAuiNotebookPageChanged > PHP panel: width = %d sash pos = %d",
                        panelSize.GetWidth(), sashPos);

                    if (panelSize.GetWidth() > sashPos) {
                        // set the sash position
                        LogDebug(
                            "MainFrame::OnAuiNotebookPageChanged > reset sash position");

                        m_rightSplitter->SetSashPosition(panelSize.GetWidth());
                    }
                }
                break;

            case 4: // Lua panel
                if (m_lua && m_rightSplitter) {
                    panelSize = m_lua->GetClientSize();
                    sashPos = m_rightSplitter->GetSashPosition();
                    LogDebug(
                        "MainFrame::OnAuiNotebookPageChanged > Lua panel: width = %d sash pos = %d",
                        panelSize.GetWidth(), sashPos);

                    if (panelSize.GetWidth() > sashPos) {
                        // set the sash position
                        LogDebug(
                            "MainFrame::OnAuiNotebookPageChanged > reset sash position");

                        m_rightSplitter->SetSashPosition(panelSize.GetWidth());
                    }
                }
                break;

            case 5: // XRC panel
                if (m_xrc && m_rightSplitter) {
                    panelSize = m_xrc->GetClientSize();
                    sashPos = m_rightSplitter->GetSashPosition();
                    LogDebug("MainFrame::OnAuiNotebookPageChanged > XRC panel: width = %d sash pos = %d",
                             panelSize.GetWidth(), sashPos);

                    if (panelSize.GetWidth() > sashPos) {
                        // set the sash position
                        LogDebug(
                            "MainFrame::OnAuiNotebookPageChanged > reset sash position");
                        m_rightSplitter->SetSashPosition(panelSize.GetWidth());
                    }
                }
                break;

            default:
                if (m_visualEdit) {
                    sashPos = m_rightSplitter->GetSashPosition();
                    if (m_rightSplitterSashPos < sashPos) {
                        // restore the sash position
                        LogDebug(
                            "MainFrame::OnAuiNotebookPageChanged > restore sash position: sash pos = %d",
                            m_rightSplitterSashPos);

                        m_rightSplitter->SetSashPosition(m_rightSplitterSashPos);
                    } else {
                        // update position
                        m_rightSplitterSashPos = sashPos;
                    }
                }
                break;
            }
        }
    }
    AppData()->GenerateCode(true);
}

void MainFrame::OnFindDialog(wxCommandEvent&)
{
    if (!m_findDialog) {
        m_findDialog = new wxFindReplaceDialog(this, &m_findData, _("Find"));
        m_findDialog->Centre(wxCENTRE_ON_SCREEN | wxBOTH);
    }
    m_findDialog->Show(true);
}

void MainFrame::OnFindClose(wxFindDialogEvent&)
{
    m_findDialog->Destroy();
    m_findDialog = 0;
}

void MainFrame::OnFind(wxFindDialogEvent& event)
{
#if 0
    for (size_t page = 0; page < m_notebook->GetPageCount(); ++page) {
        event.StopPropagation();
        event.SetClientData(m_findDialog);
        m_notebook->GetPage(page)->GetEventHandler()->ProcessEvent(event);
    }
#endif
    wxWindow* page = m_notebook->GetCurrentPage();
    if (page) {
        event.StopPropagation();
        event.SetClientData(m_findDialog);
        page->GetEventHandler()->ProcessEvent(event);
    }
}

wxMenuBar* MainFrame::CreateWeaverMenuBar()
{
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(wxID_NEW, _("&New Project\tCtrl+N"), _("Create an empty project"));
    menuFile->Append(wxID_OPEN, _("&Open...\tCtrl+O"), _("Open a project"));

    menuFile->Append(wxID_SAVE, _("&Save\tCtrl+S"), _("Save current project"));
    menuFile->Append(wxID_SAVEAS, _("Save &As...\tCtrl-Shift+S"), _("Save current project as..."));
    menuFile->AppendSeparator();
    menuFile->Append(ID_IMPORT_XRC, _("&Import XRC..."), _("Import XRC file"));
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXECUTE, _("&Generate Code\tF8"), _("Generate Code"));
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT, _("E&xit\tAlt-F4"), _("Quit wxWeaver"));

    wxMenu* menuEdit = new wxMenu;
    menuEdit->Append(wxID_UNDO, _("&Undo \tCtrl+Z"), _("Undo changes"));
    menuEdit->Append(wxID_REDO, _("&Redo \tCtrl+Y"), _("Redo changes"));
    menuEdit->AppendSeparator();
    menuEdit->Append(wxID_COPY, _("&Copy \tCtrl+C"), _("Copy selected object"));
    menuEdit->Append(wxID_CUT, _("Cut \tCtrl+X"), _("Cut selected object"));
    menuEdit->Append(wxID_PASTE, _("&Paste \tCtrl+V"), _("Paste on selected object"));
    menuEdit->Append(wxID_DELETE, _("&Delete \tCtrl+D"), _("Delete selected object"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_CLIPBOARD_COPY, _("Copy Object To Clipboard\tCtrl+Shift+C"), _("Copy Object to Clipboard"));
    menuEdit->Append(ID_CLIPBOARD_PASTE, _("Paste Object From Clipboard\tCtrl+Shift+V"), _("Paste Object from Clipboard"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_EXPAND, _("Toggle &Expand\tAlt+W"), _("Toggle wxEXPAND flag of sizeritem properties"));
    menuEdit->Append(ID_STRETCH, _("Toggle &Stretch\tAlt+S"), _("Toggle option property of sizeritem properties"));
    menuEdit->Append(wxID_UP, _("Move Up\tAlt+Up"), _("Move Up selected object"));
    menuEdit->Append(wxID_DOWN, _("Move Down\tAlt+Down"), _("Move Down selected object"));
    menuEdit->Append(wxID_BACKWARD, _("Move Left\tAlt+Left"), _("Move Left selected object"));
    menuEdit->Append(wxID_FORWARD, _("Move Right\tAlt+Right"), _("Move Right selected object"));
    menuEdit->AppendSeparator();
    menuEdit->Append(wxID_FIND, _("&Find\tCtrl+F"), _("Find text in the active code viewer"));
    menuEdit->AppendSeparator();
    menuEdit->Append(ID_ALIGN_LEFT, _("&Align &Left\tAlt+Shift+Left"), _("Align item to the left"));
    menuEdit->Append(ID_ALIGN_CENTER_H, _("&Align Center &Horizontal\tAlt+Shift+H"), _("Align item to the center horizontally"));
    menuEdit->Append(ID_ALIGN_RIGHT, _("&Align &Right\tAlt+Shift+Right"), _("Align item to the right"));
    menuEdit->Append(ID_ALIGN_TOP, _("&Align &Top\tAlt+Shift+Up"), _("Align item to the top"));
    menuEdit->Append(ID_ALIGN_CENTER_V, _("&Align Center &Vertical\tAlt+Shift+V"), _("Align item to the center vertically"));
    menuEdit->Append(ID_ALIGN_BOTTOM, _("&Align &Bottom\tAlt+Shift+Down"), _("Align item to the bottom"));

    wxMenu* menuView = new wxMenu;
    menuView->Append(ID_PREVIEW_XRC, _("&XRC Window\tF5"), _("Show a preview of the XRC window"));
    menuView->AppendSeparator();

    if (m_style != wxWEAVER_GUI_DOCKABLE) {
        menuView->Append(
            ID_WINDOW_SWAP,
            _("&Swap The Editor and Properties Window\tF12"),
            _("Swap The Editor and Properties Window"));
    }
    wxMenu* menuTools = new wxMenu;
    menuTools->Append(ID_GEN_INHERIT_CLS, _("&Generate Inherited Class\tF6"), _("Creates the needed files and class for proper inheritance of your designed GUI"));

    wxMenu* menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT, _("&About...\tF1"), _("Show about dialog"));

    // now append the freshly created menu to the menu bar...
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(menuFile, _("&File"));
    menuBar->Append(menuEdit, _("&Edit"));
    menuBar->Append(menuView, _("&View"));
    menuBar->Append(menuTools, _("&Tools"));
    menuBar->Append(menuHelp, _("&Help"));
    return menuBar;
}

wxToolBar* MainFrame::CreateWeaverToolBar()
{
    wxToolBar* toolbar = CreateToolBar();
    toolbar->SetToolBitmapSize(wxSize(TOOL_SIZE, TOOL_SIZE));
    toolbar->AddTool(wxID_NEW, _("New Project"), AppBitmaps::GetBitmap("new", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("New Project (Ctrl+N)"), _("Start a new project."));
    toolbar->AddTool(wxID_OPEN, _("Open Project"), AppBitmaps::GetBitmap("open", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Open Project (Ctrl+O)"), _("Open an existing project."));
    toolbar->AddTool(wxID_SAVE, _("Save Project"), AppBitmaps::GetBitmap("save", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Save Project (Ctrl+S)"), _("Save the current project."));
    toolbar->AddSeparator();
    toolbar->AddTool(wxID_UNDO, _("Undo"), AppBitmaps::GetBitmap("undo", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Undo (Ctrl+Z)"), _("Undo the last action."));
    toolbar->AddTool(wxID_REDO, _("Redo"), AppBitmaps::GetBitmap("redo", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Redo (Ctrl+Y)"), _("Redo the last action that was undone."));
    toolbar->AddSeparator();
    toolbar->AddTool(wxID_CUT, _("Cut"), AppBitmaps::GetBitmap("cut", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Cut (Ctrl+X)"), _("Remove the selected object and place it on the clipboard."));
    toolbar->AddTool(wxID_COPY, _("Copy"), AppBitmaps::GetBitmap("copy", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Copy (Ctrl+C)"), _("Copy the selected object to the clipboard."));
    toolbar->AddTool(wxID_PASTE, _("Paste"), AppBitmaps::GetBitmap("paste", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Paste (Ctrl+V)"), _("Insert an object from the clipboard."));
    toolbar->AddTool(wxID_DELETE, _("Delete"), AppBitmaps::GetBitmap("delete", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Delete (Ctrl+D)"), _("Remove the selected object."));
    toolbar->AddSeparator();
    toolbar->AddTool(wxID_EXECUTE, _("Generate Code"), AppBitmaps::GetBitmap("generate", TOOL_SIZE), wxNullBitmap, wxITEM_NORMAL, _("Generate Code (F8)"), _("Create code from the current project."));
    toolbar->AddSeparator();
    toolbar->AddTool(ID_ALIGN_LEFT, "", AppBitmaps::GetBitmap("lalign", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Align Left"), _("The item will be aligned to the left of the space alotted to it by the sizer."));
    toolbar->AddTool(ID_ALIGN_CENTER_H, "", AppBitmaps::GetBitmap("chalign", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Align Center Horizontally"), _("The item will be centered horizontally in the space alotted to it by the sizer."));
    toolbar->AddTool(ID_ALIGN_RIGHT, "", AppBitmaps::GetBitmap("ralign", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Align Right"), _("The item will be aligned to the right of the space alotted to it by the sizer."));
    toolbar->AddSeparator();
    toolbar->AddTool(ID_ALIGN_TOP, "", AppBitmaps::GetBitmap("talign", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Align Top"), _("The item will be aligned to the top of the space alotted to it by the sizer."));
    toolbar->AddTool(ID_ALIGN_CENTER_V, "", AppBitmaps::GetBitmap("cvalign", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Align Center Vertically"), _("The item will be centered vertically within space alotted to it by the sizer."));
    toolbar->AddTool(ID_ALIGN_BOTTOM, "", AppBitmaps::GetBitmap("balign", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Align Bottom"), _("The item will be aligned to the bottom of the space alotted to it by the sizer."));
    toolbar->AddSeparator();
    toolbar->AddTool(ID_EXPAND, "", AppBitmaps::GetBitmap("expand", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Expand (Alt+W)"), _("The item will be expanded to fill the space assigned to the item."));
    toolbar->AddTool(ID_STRETCH, "", AppBitmaps::GetBitmap("stretch", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Stretch (Alt+S)"), _("The item will grow and shrink with the sizer."));
    toolbar->AddSeparator();
    toolbar->AddTool(ID_BORDER_LEFT, "", AppBitmaps::GetBitmap("left", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Left Border"), _("A border will be added on the left side of the item."));
    toolbar->AddTool(ID_BORDER_RIGHT, "", AppBitmaps::GetBitmap("right", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Right Border"), _("A border will be  added on the right side of the item."));
    toolbar->AddTool(ID_BORDER_TOP, "", AppBitmaps::GetBitmap("top", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Top Border"), _("A border will be  added on the top of the item."));
    toolbar->AddTool(ID_BORDER_BOTTOM, "", AppBitmaps::GetBitmap("bottom", TOOL_SIZE), wxNullBitmap, wxITEM_CHECK, _("Bottom Border"), _("A border will be  added on the bottom of the item."));

    if (m_style != wxWEAVER_GUI_DOCKABLE) {
        toolbar->AddSeparator();
        toolbar->AddTool(
            ID_WINDOW_SWAP, "", AppBitmaps::GetBitmap("swap", TOOL_SIZE),
            wxNullBitmap, wxITEM_NORMAL,
            _("Swap The Editor and Properties Window (F12)"),
            _("Swap the design window and properties window."));
    }
    toolbar->Realize();
    return toolbar;
}

wxWindow* MainFrame::CreateDesignerWindow(wxWindow* parent)
{
    m_notebook = new wxAuiNotebook(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_TOP);
    m_notebook->SetArtProvider(new AuiTabArt());

    m_visualEdit = new VisualEditor(m_notebook);
    AppData()->GetManager()->SetVisualEditor(m_visualEdit);

    m_notebook->AddPage(m_visualEdit, _("Designer"), false, 0);
    m_notebook->SetPageBitmap(0, AppBitmaps::GetBitmap("designer", 16));

    m_cpp = new CppPanel(m_notebook, wxID_ANY);
    m_notebook->AddPage(m_cpp, "C++", false, 1);
    m_notebook->SetPageBitmap(1, AppBitmaps::GetBitmap("c++", 16));

    m_python = new PythonPanel(m_notebook, wxID_ANY);
    m_notebook->AddPage(m_python, "Python", false, 2);
    m_notebook->SetPageBitmap(2, AppBitmaps::GetBitmap("python", 16));

    m_php = new PHPPanel(m_notebook, wxID_ANY);
    m_notebook->AddPage(m_php, "PHP", false, 3);
    m_notebook->SetPageBitmap(3, AppBitmaps::GetBitmap("php", 16));

    m_lua = new LuaPanel(m_notebook, wxID_ANY);
    m_notebook->AddPage(m_lua, "Lua", false, 4);
    m_notebook->SetPageBitmap(4, AppBitmaps::GetBitmap("lua", 16));

    m_xrc = new XrcPanel(m_notebook, wxID_ANY);
    m_notebook->AddPage(m_xrc, "XRC", false, 5);
    m_notebook->SetPageBitmap(5, AppBitmaps::GetBitmap("xrc", 16));

    return m_notebook;
}

wxWindow* MainFrame::CreateComponentPalette(wxWindow* parent)
{
    // la paleta de componentes, no es un observador propiamente dicho, ya
    // que no responde ante los eventos de la aplicación
    m_palette = new Palette(parent, wxID_ANY);
    m_palette->Create();
#if 0
    m_palette->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
#endif
    return m_palette;
}

wxWindow* MainFrame::CreateObjectTree(wxWindow* parent)
{
    m_objTree = new ObjectTree(parent, wxID_ANY);
    m_objTree->Create();
    return m_objTree;
}

wxWindow* MainFrame::CreateObjectInspector(wxWindow* parent)
{
    //TO-DO: make object inspector style selectable.
    int style = (m_style == wxWEAVER_GUI_CLASSIC
                     ? wxWEAVER_OI_MULTIPAGE_STYLE
                     : wxWEAVER_OI_SINGLE_PAGE_STYLE);
    m_objInsp = new ObjectInspector(parent, wxID_ANY, style);
    return m_objInsp;
}

void MainFrame::CreateWideGui()
{
    m_style = wxWEAVER_GUI_WIDE;

    // MainFrame only contains m_leftSplitter window
    m_leftSplitter
        = new wxSplitterWindow(
            this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    wxWindow* objectTree
        = Title::CreateTitle(CreateObjectTree(m_leftSplitter), _("Project"));

    // panel1 contains palette and splitter2 (m_rightSplitter)
    wxPanel* panel1 = new wxPanel(m_leftSplitter, wxID_ANY);

    wxWindow* palette = Title::CreateTitle(
        CreateComponentPalette(panel1), _("Widgets"));

    m_rightSplitter
        = new wxSplitterWindow(
            panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    wxBoxSizer* panel1_sizer = new wxBoxSizer(wxVERTICAL);
    panel1_sizer->Add(palette, 0, wxEXPAND);
    panel1_sizer->Add(m_rightSplitter, 1, wxEXPAND);
    panel1->SetSizer(panel1_sizer);

    // splitter2 contains the editor and the object inspector
    wxWindow* designer
        = Title::CreateTitle(CreateDesignerWindow(m_rightSplitter), _("Editor"));

    wxWindow* objectInspector
        = Title::CreateTitle(CreateObjectInspector(m_rightSplitter), _("Properties"));

    m_leftSplitter->SplitVertically(objectTree, panel1, m_leftSplitterWidth);

    // Need to update the left splitter so the right one is drawn correctly
    wxSizeEvent update(GetSize(), GetId());
    ProcessEvent(update);
    m_leftSplitter->UpdateSize();
    m_leftSplitter->SetMinimumPaneSize(2);

    //modified by tyysoft to restore the last layout.
    if (m_rightSplitterType == _("editor")) {
        m_rightSplitter->SplitVertically(
            designer, objectInspector, m_rightSplitterWidth);
    } else {
        m_rightSplitter->SplitVertically(
            objectInspector, designer, m_rightSplitterWidth);
    }
    m_rightSplitter->SetSashGravity(1);
    m_rightSplitter->SetMinimumPaneSize(2);

    SetMinSize(wxSize(700, 380));
}

void MainFrame::CreateClassicGui()
{
    m_style = wxWEAVER_GUI_CLASSIC;

    // Give ID to left splitter
    m_leftSplitter = new wxSplitterWindow(
        this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    m_rightSplitter = new wxSplitterWindow(
        m_leftSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);

    wxWindow* objectTree
        = Title::CreateTitle(
            CreateObjectTree(m_rightSplitter), _("Project"));

    wxWindow* objectInspector
        = Title::CreateTitle(
            CreateObjectInspector(m_rightSplitter), _("Properties"));

    // panel1 contains palette and designer
    wxPanel* panel1 = new wxPanel(m_leftSplitter, wxID_ANY);

    wxWindow* palette = Title::CreateTitle(
        CreateComponentPalette(panel1), _("Widgets"));

    wxWindow* designer = Title::CreateTitle(
        CreateDesignerWindow(panel1), _("Editor"));

    wxBoxSizer* panel1_sizer = new wxBoxSizer(wxVERTICAL);
    panel1_sizer->Add(palette, 0, wxEXPAND);
    panel1_sizer->Add(designer, 1, wxEXPAND);
    panel1->SetSizer(panel1_sizer);

    m_leftSplitter->SplitVertically(
        m_rightSplitter, panel1, m_leftSplitterWidth);

    // Need to update the left splitter so the right one is drawn correctly
    wxSizeEvent update(GetSize(), GetId());
    ProcessEvent(update);
    m_leftSplitter->UpdateSize();

    m_rightSplitter->SplitHorizontally(
        objectTree, objectInspector, m_rightSplitterWidth);

    m_rightSplitter->SetMinimumPaneSize(2);
    SetMinSize(wxSize(700, 465));
}

void MainFrame::CreateDockableGui()
{
    m_style = wxWEAVER_GUI_DOCKABLE;
    m_panel = new wxPanel(this, wxID_ANY);
    m_mgr.SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_LIVE_RESIZE); // TODO: Set in preferences
    m_mgr.SetManagedWindow(m_panel);
    m_mgr.SetArtProvider(new DockArt());

    wxWindow* palette = CreateComponentPalette(m_panel);
    wxWindow* treeView = CreateObjectTree(m_panel);
    wxWindow* designer = CreateDesignerWindow(m_panel);
    wxWindow* propertyEditor = CreateObjectInspector(m_panel);

    int paletteHeight = palette->GetBestSize().GetHeight();

    // clang-format off
#ifdef wxWEAVER_DEBUG
        wxWindow* dbgWnd = (wxWindow*)AppData()->GetDebugWindow(m_panel);
        m_mgr.AddPane(dbgWnd, wxAuiPaneInfo().Bottom()
                    .Name("logwindow").Caption(_("Logger"))
                    .CloseButton(false).MaximizeButton(true)
                    .LeftDockable(false).RightDockable(false)
                    .MinSize(-1, 120).FloatingSize(300, 120)
    );
#endif
    m_mgr.AddPane(designer, wxAuiPaneInfo().Center().DockFixed(true)
                    .Name("editor").Caption(_("Editor"))
                    .CloseButton(false).MaximizeButton(true)
    );
    m_mgr.AddPane(palette, wxAuiPaneInfo().Top()//.DockFixed(true)
                    .Name("palette").Caption(_("Widgets"))
                    .CloseButton(false)
                    .LeftDockable(false).RightDockable(false)
                    .MinSize(-1, paletteHeight).FloatingSize(300, paletteHeight)
    );
    m_mgr.AddPane(treeView, wxAuiPaneInfo().Left()
                    .Name("tree").Caption(_("Project"))
                    .CloseButton(false).MaximizeButton(true)
                    .TopDockable(false).BottomDockable(false)
                    .MinSize(150,-1).FloatingSize(150, 300)
    );
    m_mgr.AddPane(propertyEditor, wxAuiPaneInfo().Right()
                    .Name("propertyeditor").Caption(_("Properties"))
                    .CloseButton(false).MaximizeButton(true)
                    .MinSize(150,-1).FloatingSize(150, 300)
    );
    // clang-format on
    m_mgr.Update();
}

void MainFrame::OnIdle(wxIdleEvent&)
{
    if (m_leftSplitter)
        m_leftSplitter->SetSashPosition(m_leftSplitterWidth);

    if (m_rightSplitter)
        m_rightSplitter->SetSashPosition(m_rightSplitterWidth);

    Unbind(wxEVT_IDLE, &MainFrame::OnIdle, this);

    if (m_autoSash) {
        m_rightSplitterSashPos = m_rightSplitter->GetSashPosition();
        m_rightSplitter->Bind(
            wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED,
            &MainFrame::OnSplitterChanged, this);
    }
}

void MainFrame::OnSplitterChanged(wxSplitterEvent& event)
{
    LogDebug("MainFrame::OnSplitterChanged > pos = %d", event.GetSashPosition());
    m_rightSplitterSashPos = event.GetSashPosition(); // update position
}

void MainFrame::OnWindowSwap(wxCommandEvent&)
{
    wxWindow* win1 = m_rightSplitter->GetWindow1();
    wxWindow* win2 = m_rightSplitter->GetWindow2();

    int pos = m_rightSplitter->GetSashPosition();
    wxSize sz = m_rightSplitter->GetClientSize();

    m_rightSplitter->Unsplit();
    m_rightSplitter->SplitVertically(win2, win1);
    m_rightSplitter->SetSashPosition(sz.GetWidth() - pos);
}
