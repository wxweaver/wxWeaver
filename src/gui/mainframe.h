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

#include <wx/aui/auibook.h>
#include <wx/aui/framemanager.h>
#include <wx/fdrepdlg.h>
#include <wx/frame.h>
#include <wx/preferences.h>
#include <wx/scopedptr.h>
#include <wx/splitter.h>

class wxWeaverEvent;
class wxWeaverObjectEvent;
class wxWeaverPropertyEvent;
class wxWeaverEventHandlerEvent;

class DebugWindow;
class CppPanel;
class PythonPanel;
class LuaPanel;
class PHPPanel;
class XrcPanel;
class ObjectTree;
class ObjectInspector;
class Palette;
class VisualEditor;

class wxLog;
class wxPanel;

// TODO: Make GUI configurable in the settings dialog
/** wxWeaver GUI styles.
 */
enum {
    wxWEAVER_GUI_DOCKABLE,
    wxWEAVER_GUI_CLASSIC,
    wxWEAVER_GUI_WIDE
};

class MainFrame : public wxFrame {
public:
    MainFrame(wxWindow* parent = nullptr, int id = wxID_ANY,
              int style = wxWEAVER_GUI_DOCKABLE, wxPoint pos = wxDefaultPosition,
              wxSize size = wxDefaultSize);
    ~MainFrame() override;

    void LoadSettings();
    void SaveSettings();

    void OnPreferences(wxCommandEvent&);

    void OnSaveProject(wxCommandEvent& event);
    void OnSaveAsProject(wxCommandEvent& event);
    void OnOpenProject(wxCommandEvent& event);
    void OnNewProject(wxCommandEvent& event);
    void OnGenerateCode(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnImportXrc(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnPaste(wxCommandEvent& event);
    void OnCut(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnClipboardCopy(wxCommandEvent& e);
    void OnClipboardPaste(wxCommandEvent& e);
    void OnClipboardPasteUpdateUI(wxUpdateUIEvent& e);
    void OnToggleExpand(wxCommandEvent& event);
    void OnToggleStretch(wxCommandEvent& event);
    void OnMoveUp(wxCommandEvent& event);
    void OnMoveDown(wxCommandEvent& event);
    void OnMoveLeft(wxCommandEvent& event);
    void OnMoveRight(wxCommandEvent& event);
    void OnChangeAlignment(wxCommandEvent& event);
    void OnChangeBorder(wxCommandEvent& e);
    void OnXrcPreview(wxCommandEvent& e);
    void OnGenInhertedClass(wxCommandEvent& e);
    void OnWindowSwap(wxCommandEvent& e);

    void OnAuiNotebookPageChanged(wxAuiNotebookEvent& event);

    void OnProjectLoaded(wxWeaverEvent& event);
    void OnProjectSaved(wxWeaverEvent& event);
    void OnObjectExpanded(wxWeaverObjectEvent& event);
    void OnObjectSelected(wxWeaverObjectEvent& event);
    void OnObjectCreated(wxWeaverObjectEvent& event);
    void OnObjectRemoved(wxWeaverObjectEvent& event);
    void OnPropertyModified(wxWeaverPropertyEvent& event);
    void OnEventHandlerModified(wxWeaverEventHandlerEvent& event);
    void OnCodeGeneration(wxWeaverEvent& event);
    void OnProjectRefresh(wxWeaverEvent& event);

    void OnSplitterChanged(wxSplitterEvent& event);

    void InsertRecentProject(const wxString& file);

    wxWindow* CreateComponentPalette(wxWindow* parent);
    wxWindow* CreateDesignerWindow(wxWindow* parent);
    wxWindow* CreateObjectTree(wxWindow* parent);
    wxWindow* CreateObjectInspector(wxWindow* parent);
    wxMenuBar* CreateWeaverMenuBar();
    wxToolBar* CreateWeaverToolBar();

    void CreateClassicGui();
    void CreateDockableGui();
    void CreateWideGui();

    void OnFindDialog(wxCommandEvent& event);
    void OnFind(wxFindDialogEvent& event);
    void OnFindClose(wxFindDialogEvent& event);

    bool SaveWarning();

private:
    void UpdateFrame();
    void UpdateLayoutTools();
    void UpdateRecentProjects(); // Actualiza los projectos más recientes en el menu

    void OnOpenRecent(wxCommandEvent& event);
    void OnIdle(wxIdleEvent&); // Used to correctly restore splitter position

    ObjectTree* m_objTree;
    ObjectInspector* m_objInsp;
    VisualEditor* m_visualEdit;
    Palette* m_palette;
    PythonPanel* m_python;
    CppPanel* m_cpp;
    LuaPanel* m_lua;
    PHPPanel* m_php;
    XrcPanel* m_xrc;

    wxAuiManager m_mgr;
    wxPanel* m_panel;
    wxAuiNotebook* m_notebook;
    wxEvtHandler* m_focusKillEvtHandler; // Used to force propgrid to save on lost focus
    wxFindReplaceData m_findData;
    wxFindReplaceDialog* m_findDialog;
    wxSplitterWindow* m_leftSplitter;
    wxSplitterWindow* m_rightSplitter;
    wxString m_rightSplitterType;
    wxString m_currentDir;
    wxString m_recentProjects[4];

    wxScopedPtr<wxPreferencesEditor> m_prefsEditor;

    bool m_autoSash;            // Automatically update sash in splitter window base on user action
    int m_pageSelection;        // Save which page is selected
    int m_rightSplitterSashPos; // Save right splitter's sash position
    int m_leftSplitterWidth;
    int m_rightSplitterWidth;
    int m_style;
};
